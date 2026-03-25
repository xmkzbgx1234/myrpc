#pragma once

namespace myrpc {
enum ErrorCode {
    RPC_OK = 0,
    // 1000-1099: protocol / packet errors
    RPC_INVALID_PACKET = 1000,
    RPC_INVALID_MAGIC = 1001,
    RPC_UNSUPPORTED_VERSION = 1002,
    RPC_INCOMPLETE_HEADER = 1003,
    RPC_INCOMPLETE_BODY = 1004,
    RPC_HEADER_PARSE_ERROR = 1005,
    RPC_REQUEST_PARSE_ERROR = 1006,
    RPC_RESPONSE_PARSE_ERROR = 1007,
    // 1100-1199: routing errors
    RPC_SERVICE_NOT_FOUND = 1100,
    RPC_METHOD_NOT_FOUND = 1101,
    // 1200-1299: client / network errors
    RPC_CREATE_SOCKET_FAILED = 1200,
    RPC_CONNECT_FAILED = 1201,
    RPC_SEND_FAILED = 1202,
    RPC_RECV_FAILED = 1203,
    RPC_REQUEST_ID_MISMATCH = 1204,
    // 1300-1399: serialization / deserialization errors
    RPC_REQUEST_SERIALIZE_ERROR = 1300,
    RPC_RESPONSE_SERIALIZE_ERROR = 1301,
    // 1400-1499: server internal errors
    RPC_INTERNAL_ERROR = 1400,
};
}  // namespace myrpc
