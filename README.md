# RPC sync

Two processes communication with `tirpc` RPC calls.

# How to

1. Make sure you have `rpcgen` and `libtirpc` installed. For Ubuntu:

```
sudo apt update && sudo apt upgrade
sudo apt install g++ rpcsvc-proto libtirpc-dev
```

2. Start `rpcbind` service:
```
sudo systemctl start rpcbind
sudo systemctl status rpcbind
```

3. Use `rpcgen` to generate sources:

```
rpcgen -C -M -a state.x
```

4. Find there yoor tiprc includes are located and build the app. Likely they are in `/usr/include/tirpc`:

```
g++ -o app app.cpp state_clnt.c state_xdr.c -I /usr/include/tirpc -ltirpc
```

5. Run in two separate terminals:

```
./app sleep
./app run
```

6. Make sure two processes communicate via rpc by stopping one of them and watching other's behavior:

```
localhost: RPC: Unable to receive; errno = Connection reset by peer
```