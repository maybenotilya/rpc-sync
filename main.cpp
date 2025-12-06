#include <chrono>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "rpc_sync.grpc.pb.h"

using namespace grpc;

RPCSync::State State;

class PingServiceImpl final : public RPCSync::PingService::Service {
public:
    Status Ping(ServerContext* context, RPCSync::PingRequest const* request,
                RPCSync::PingResponse* response) override {
        std::cout << "Received ping from other process" << std::endl;

        if (State == RPCSync::State::Sleep) {
            State = RPCSync::State::Run;
            std::cout << "State changed: sleep -> run" << std::endl;
        } else {
            State = RPCSync::State::Sleep;
            std::cout << "State changed: run -> sleep" << std::endl;
        }

        return Status::OK;
    }
};

class PingClient {
public:
    PingClient(std::shared_ptr<Channel> channel) : stub_(RPCSync::PingService::NewStub(channel)) {}

    bool SendPing() {
        RPCSync::PingRequest request;

        RPCSync::PingResponse response;
        ClientContext context;

        std::chrono::system_clock::time_point deadline =
                std::chrono::system_clock::now() + std::chrono::seconds(5);
        context.set_deadline(deadline);

        Status status = stub_->Ping(&context, request, &response);

        if (status.ok()) {
            return true;
        } else {
            return false;
        }
    }

private:
    std::unique_ptr<RPCSync::PingService::Stub> stub_;
};

void RunServer(std::string const& address) {
    PingServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << address << std::endl;

    server->Wait();
}

void RunClient(std::string const& address) {
    // Wait for server to run
    std::this_thread::sleep_for(std::chrono::seconds(1));

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (State == RPCSync::State::Run) {
            std::cout << "Sending ping" << std::endl;

            auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
            PingClient client(channel);

            bool success = client.SendPing();

            if (success) {
                State = RPCSync::State::Sleep;
                std::cout << "State changed: run -> sleep" << std::endl;
            } else {
                std::cout << "Failed to send ping, retrying..." << std::endl;
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <clientPort> <targetPort> <initialState>"
                  << std::endl;
        std::cerr << "Example: " << argv[0] << " 1234 4321 sleep" << std::endl;
        return 1;
    }

    std::string clientPort = argv[1];
    std::string targetPort = argv[2];
    std::string initialState = argv[3];

    if (initialState == "run") {
        State = RPCSync::State::Run;
    } else if (initialState == "sleep") {
        State = RPCSync::State::Sleep;
    } else {
        std::cerr << "Initial state must be 'run' or 'sleep'" << std::endl;
        return 1;
    }

    std::cout << "Starting with state: " << initialState << std::endl;
    std::cout << "Client port: " << clientPort << ", Target port: " << targetPort << std::endl;

    std::string clientAddress = "localhost:" + clientPort;
    std::string targetAddress = "localhost:" + targetPort;

    std::thread server_thread([&clientAddress]() { RunServer(clientAddress); });
    server_thread.detach();

    RunClient(targetAddress);

    return 0;
}
