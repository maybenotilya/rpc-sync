#include <arpa/inet.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <rpc/rpc.h>
#include <string>
#include <thread>
#include <unistd.h>

#include "state.h"

enum State : uint8_t { Sleep = 0, Run = 1 };

State state;
State startingState;
char localhost[] = "localhost";

bool IsRun() {
    return state == State::Run;
}

int GetSelfProg() {
    return startingState == State::Run ? STATE_PROG + 1 : STATE_PROG;
}

int GetOtherProg() {
    return startingState == State::Sleep ? STATE_PROG + 1 : STATE_PROG;
}

#undef STATE_PROG

void* state_proc_1_svc(void*, struct svc_req*) {
    if (state != State::Sleep) {
        return nullptr;
    }
    std::cout << "Message recieved" << std::endl;
    state = State::Run;
    std::cout << "State changed: " << (state ? "Run" : "Sleep") << std::endl;

    return nullptr;
}

bool SendMessage() {
    CLIENT* clnt;

    clnt = clnt_create(localhost, GetOtherProg(), STATE_VERS, "tcp");
    if (clnt == nullptr) {
        clnt_pcreateerror(localhost);
        return false;
    }

    if (state_proc_1(nullptr, clnt) == nullptr) {
        std::cerr << "Failed to send message" << std::endl;

        return false;
    }

    clnt_destroy(clnt);
    return true;
}

void state_prog_1(struct svc_req* rqstp, SVCXPRT* transp) {
    union {
        int fill;
    } argument;

    char* result;
    xdrproc_t _xdr_argument, _xdr_result;
    char* (*local)(char*, struct svc_req*);

    switch (rqstp->rq_proc) {
        case NULLPROC:
            (void)svc_sendreply(transp, (xdrproc_t)xdr_void, (char*)NULL);
            return;

        case STATE_PROC:
            _xdr_argument = (xdrproc_t)xdr_void;
            _xdr_result = (xdrproc_t)xdr_void;
            local = (char* (*)(char*, struct svc_req*))state_proc_1_svc;
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
    result = (*local)((char*)&argument, rqstp);
    if (result != NULL && !svc_sendreply(transp, (xdrproc_t)_xdr_result, result)) {
        svcerr_systemerr(transp);
    }
    if (!svc_freeargs(transp, (xdrproc_t)_xdr_argument, (caddr_t)&argument)) {
        fprintf(stderr, "%s", "unable to free arguments");
        exit(1);
    }
    return;
}

void rpc_server() {
    SVCXPRT* transp;

    pmap_unset(GetSelfProg(), STATE_VERS);

    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == nullptr) {
        std::cerr << "Failed to create transport handle" << std::endl;
        return;
    }

    if (!svc_register(transp, GetSelfProg(), STATE_VERS, state_prog_1, IPPROTO_TCP)) {
        std::cerr << "Failed to register server" << std::endl;
        return;
    }

    svc_run();

    return;
}

void StartSync() {
    std::thread server_thread(rpc_server);
    server_thread.detach();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    while (true) {
        if (IsRun()) {
            std::cout << "Sending message" << std::endl;
            if (!SendMessage()) {
                std::cout << "Failed to send" << std::endl;
                continue;
            }
            state = State::Sleep;
            std::cout << "State changed: " << (state ? "Run" : "Sleep") << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [sleep|run]" << std::endl;
        return 1;
    }

    if (strcmp(argv[1], "sleep") == 0) {
        state = State::Sleep;
    } else if (strcmp(argv[1], "run") == 0) {
        state = State::Run;
    } else {
        std::cout << "Unrecognized state: " << argv[1] << std::endl;
        return 1;
    }

    startingState = state;

    StartSync();

    return 0;
}