# RPC sync

rpcgen -C -M -a state.x

g++ -o app app.cpp state_clnt.c state_xdr.c -I /usr/include/tirpc -ltirpc