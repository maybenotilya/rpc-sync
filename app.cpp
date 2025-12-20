#include <chrono>
#include <iostream>
#include <mutex>
#include <rpc/rpc.h>
#include <thread>

#include "state.h"

#define RUN_STATE_PROG 0x20000002
#define SLEEP_STATE_PROG 0x20000001

int const MAX_RETRIES = 10;

char* startingState = "sleep";

int GetSelfStateProg() {
    if (strcmp(startingState, "sleep") == 0) {
        return SLEEP_STATE_PROG;
    } else {
        return RUN_STATE_PROG;
    }
}

int GetOtherStateProg() {
    if (strcmp(startingState, "sleep") == 0) {
        return RUN_STATE_PROG;
    } else {
        return SLEEP_STATE_PROG;
    }
}

#undef RUN_STATE_PROG
#undef SLEEP_STATE_PROG

char localhost[] = "localhost";

std::mutex mu;
enum class State { Sleep = 0, Run = 1 };
State volatile state = State::Sleep;

std::string StateToString(State volatile state) {
    return state == State::Sleep ? "Sleep" : "Run";
}

bool_t ping_1_svc(StateRequest* argp, StateResponse* result, struct svc_req* rqstp) {
    // To reduce number of outputs
    std::this_thread::sleep_for(std::chrono::seconds(1));

    mu.lock();
    state = State::Run;
    std::cout << "Ping recieved, new state: " << StateToString(state) << std::endl;
    mu.unlock();

    return TRUE;
}

int state_prog_1_freeresult(SVCXPRT* transp, xdrproc_t xdr_result, caddr_t result) {
    xdr_free(xdr_result, result);
    return 1;
}

void state_prog_1(struct svc_req* rqstp, SVCXPRT* transp) {
    union {
        StateRequest ping_1_arg;
    } argument;

    union {
        StateResponse ping_1_res;
    } result;

    bool_t retval;
    xdrproc_t _xdr_argument, _xdr_result;
    bool_t (*local)(char*, void*, struct svc_req*);

    switch (rqstp->rq_proc) {
        case NULLPROC:
            (void)svc_sendreply(transp, (xdrproc_t)xdr_void, (char*)NULL);
            return;

        case Ping:
            _xdr_argument = (xdrproc_t)xdr_StateRequest;
            _xdr_result = (xdrproc_t)xdr_StateResponse;
            local = (bool_t (*)(char*, void*, struct svc_req*))ping_1_svc;
            break;

        default:
            svcerr_noproc(transp);
            return;
    }
    memset((char*)&argument, 0, sizeof(argument));
    if (!svc_getargs(transp, (xdrproc_t)_xdr_argument, (caddr_t)&argument)) {
        svcerr_decode(transp);
        return;
    }
    retval = (bool_t)(*local)((char*)&argument, (void*)&result, rqstp);
    if (retval > 0 && !svc_sendreply(transp, (xdrproc_t)_xdr_result, (char*)&result)) {
        svcerr_systemerr(transp);
    }
    if (!svc_freeargs(transp, (xdrproc_t)_xdr_argument, (caddr_t)&argument)) {
        fprintf(stderr, "%s", "unable to free arguments");
        exit(1);
    }
    if (!state_prog_1_freeresult(transp, _xdr_result, (caddr_t)&result))
        fprintf(stderr, "%s", "unable to free results");

    return;
}

void RunClient() {
    // Wait for other server to init
    std::this_thread::sleep_for(std::chrono::seconds(2));

    CLIENT* client = clnt_create(localhost, GetOtherStateProg(), STATE_VERS, "tcp");
    int attempt = 0;
    while (!client) {
        std::cout << "Failed to create client attempt " << ++attempt << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (attempt >= MAX_RETRIES) {
            std::cout << "Failed to create client within " << MAX_RETRIES << " attempts"
                      << std::endl;
            return;
        }
        client = clnt_create(localhost, GetOtherStateProg(), STATE_VERS, "tcp");
    }
    if (!client) {
        clnt_pcreateerror(localhost);
        return;
    }

    StateRequest req;
    StateResponse response;

    auto status = ping_1(&req, &response, client);
    if (status != RPC_SUCCESS) {
        clnt_perror(client, localhost);
        return;
    }

    while (true) {
        // Wait until we become Run
        while (true) {
            // Lock to take state snapshot
            mu.lock();
            if (state == State::Run) {
                // If Run, change to sleep and execute clientside request
                state = State::Sleep;
                mu.unlock();
                break;
            } else {
                // If Sleep, continue waiting
                mu.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        std::cout << "Ping sent, new state: " << StateToString(state) << std::endl;
        status = ping_1(&req, &response, client);
        if (status != RPC_SUCCESS) {
            clnt_perror(client, localhost);
            return;
        }
    }
}

void RunServer() {
    SVCXPRT* transp;
    pmap_unset(GetSelfStateProg(), STATE_VERS);

    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (!transp) {
        std::cout << "Failed to create transport" << std::endl;
        return;
    }

    if (!svc_register(transp, GetSelfStateProg(), STATE_VERS, state_prog_1, IPPROTO_TCP)) {
        std::cout << "Failed to register server" << std::endl;
        return;
    }

    svc_run();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: ./app <sleep|run>" << std::endl;
        return 1;
    }

    if (strcmp(argv[1], "sleep") == 0 || strcmp(argv[1], "run") == 0) {
        startingState = argv[1];
    } else {
        std::cout << "Wrong starting state: " << argv[1] << std::endl;
        return 1;
    }

    std::thread clientThread(RunClient);
    clientThread.detach();

    RunServer();

    return 0;
}
