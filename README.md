# myrpc

A small C++ RPC framework project built with Protobuf, Muduo, Redis, and CMake.

`myrpc` focuses on the core pieces of an RPC framework: protocol design, client/server invocation, framework-level error handling, and service registration/discovery.

## Highlights

- Custom RPC request/response headers with `magic`, `version`, `request_id`, `body_len`, `error_code`, and `error_msg`
- Client invocation based on `google::protobuf::RpcChannel` and `RpcController`
- Muduo-based provider runtime
- Redis-backed service registration and discovery
- Role-based app config for provider and consumer
- Build-time Protobuf code generation from `proto/`
- Multi-service examples with `UserService` and `FriendService`

## Architecture

The current flow is:

```text
Consumer Stub
  -> MyRpcChannel
  -> Redis service discovery
  -> TCP request send
  -> MyRpcProvider
  -> protobuf service dispatch
  -> TCP response send
  -> Consumer response parse
```

At this stage, the framework uses a synchronous short-connection model on the consumer side and a Muduo event-driven model on the provider side.

## Project Layout

```text
myrpc/
├── include/myrpc/        # public framework headers
├── src/                  # framework implementation
├── proto/                # .proto definitions
├── common/               # config and logging utilities
├── examples/consumer/    # consumer demos
├── examples/provider/    # provider demos
├── tests/                # tests and smoke checks
├── build/                # CMake build output
├── bin/                  # generated executables
└── lib/                  # generated libraries
```

## Current Features

- RPC protocol with custom request and response headers
- Framework error codes and framework error responses
- Request/response correlation through `request_id`
- Service publish and dynamic method dispatch on the provider side
- Redis-based service registration and discovery
- User service example
- Friend service example
- Build-time generation of protobuf C++ sources

## Requirements

- C++17 compiler
- CMake >= 3.16
- Protobuf and `protoc`
- Muduo and its dependencies
- Redis server
- `hiredis`

## Build

Build in WSL:

```bash
cmake -S . -B build
cmake --build build -j4
```

## Configuration

Provider config example in `server_config.ini`:

```ini
[rpcserver]
rpcserverip = 127.0.0.1
rpcserverport = 9000
magic = 0x12345678

[redis]
host = 127.0.0.1
port = 6379
password =
db = 0
```

Consumer config example in `client_config.ini`:

```ini
[consumer]
magic = 0x12345678
timeout_ms = 10000

[redis]
host = 127.0.0.1
port = 6379
password =
db = 0
```

## Quick Start

Start Redis first:

```bash
redis-server --daemonize yes
redis-cli ping
```

Start the provider:

```bash
./bin/provider -i server_config.ini -r provider
```

Run the user service consumer:

```bash
./bin/consumer -i client_config.ini -r consumer
```

Run the friend service consumer:

```bash
./bin/consumer_friend -i client_config.ini -r consumer
```

## Redis Registration Model

The current minimal registration/discovery model stores one endpoint per service:

```text
myrpc:service:UserServiceRpc   -> 127.0.0.1:9000
myrpc:service:FriendServiceRpc -> 127.0.0.1:9000
```

This is enough for the first version of service discovery. Multi-instance registration, TTL renewal, and load balancing are left for later stages.

## Protocol Notes

Request packet:

```text
4-byte header_len + request_header + request_body
```

Response packet:

```text
4-byte header_len + response_header + response_body
```

Framework errors are returned through `RpcResponseHeader.error_code` and `error_msg`.
Business errors stay inside the protobuf response body.

## Example Services

- `UserServiceRpc`
  - `Login`
  - `Register`
- `FriendServiceRpc`
  - `GetFriendList`

## Current Limitations

- Consumer still uses a synchronous short-connection model
- Redis registration is single-endpoint only
- No TTL heartbeat or automatic offline cleanup yet
- No multi-instance load balancing yet
- Benchmark and systematic tests are still limited

## Next Directions

- Add Redis TTL and heartbeat renewal
- Support multi-instance registration and discovery
- Add random/round-robin load balancing
- Improve provider-side stream/frame decoding further
- Add benchmark and protocol-level tests
