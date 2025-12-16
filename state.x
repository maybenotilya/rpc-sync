struct StateRequest {
    int placeholder;
};

struct StateResponse {
    int status;
};

program STATE_PROG {
    version STATE_VERS {
        StateResponse Ping(StateRequest) = 1;
    } = 1;
} = 0x20000001;