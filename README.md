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


- 协议层还不完整 √
  - 现在有 magicersion/request_id/body_len，比以前好多了
  - 但还缺更完整的帧设计和统一错误语义，尤其是 provider 侧框架错误回包还不完整
  - 目前仍偏“单次收发模型”，不是成熟的可扩展协议层
- 网络与并发模型还比较初级
  - client 还是同步阻塞、短连接、一次 send/recv
  - 没有超时控制、重试策略、连接复用、连接池
  - provider 虽然用了 muduo，但业务处理、线程模型、资源生命周期还没形成一套清晰设计
- 注册发现还没做
  - 这是你目标里很重要的一块
  - 现在仍然是 config.ini 里写死服务地址
  - 还没有服务注册、服务发现、负载均衡
- 框架错误处理还不够完整 √
  - consumer 已经能解析 RpcResponseHeader
  - 但 provider 对 service not found、method not found、请求解析失败这类情况，还没有系统性返回框架错误响应
  - controller、header error、业务 response 三层边界还需要彻底统一
- 目录工程化已经明显进步，但还没完全收口
  - 现在 include/、src/、proto/、examples/ 已经比之前好很多
  - 但 common/ 还比较散，公共模块边界不够明确
  - protobuf 生成规则有了，但还可以继续抽成更干净的 cmake 模块
- 示例层还不够支撑“完整项目目标”
  - 现在本质上还是围绕一个 UserService
  - 如果按你的目标，至少还该再有一个服务，比如 FriendService
  - 这样才能更像真正的“多服务 RPC 框架”
- 测试和 benchmark 基本还没成体系
  - 现在只有 protobuf smoke test
  - 还缺协议解析测试、错误路径测试、provider 路由测试、并发压测、延迟/QPS benchmark
