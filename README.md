# myrpc

A small C++ RPC framework demo built with Protobuf, Muduo, and CMake.

## Features

- RPC client and server flow based on `google::protobuf::RpcChannel` and `RpcController`
- Custom request/response headers with `magic`, `version`, `request_id`, `body_len`, and framework error fields
- Muduo-based provider runtime
- Protobuf code generated at build time from `proto/`
- Demo consumer/provider programs under `examples/`

## Project Layout

```text
myrpc/
├── include/myrpc/        # public framework headers
├── src/                  # framework implementation
├── proto/                # .proto definitions
├── common/               # config, logging, status utilities
├── examples/consumer/    # client demo
├── examples/provider/    # server demo
├── tests/                # tests and smoke checks
├── build/                # CMake build output
├── bin/                  # generated executables
└── lib/                  # generated libraries
```

## Build

Requirements:

- C++17 compiler
- CMake >= 3.16
- Protobuf and `protoc`
- Muduo and its dependencies

Build in WSL:

```bash
cmake -S . -B build
cmake --build build
```

## Run Demo

Start the provider first:

```bash
./bin/provider -i config.ini
```

Run the consumer in another terminal:

```bash
./bin/consumer -i config.ini
```

Default server config is defined in `config.ini`:

```ini
[rpcserver]
rpcserverip = 127.0.0.1
rpcserverport = 9000
magic = 0x12345678
```

## Notes

- Framework-level protocol files live in `proto/rpcHeader.proto`
- Demo service protocol lives in `proto/user.proto`
- Generated protobuf sources are placed in the build directory during compilation
