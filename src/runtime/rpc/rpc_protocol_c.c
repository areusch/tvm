#include "rpc_protocol_c.h"


const char* tvm_rpc__status_to_string(RPCStatus status) {
  switch (status) {
  case tvm_rpc__rpc_status_Success:
    return "kSuccess";
  case tvm_rpc__rpc_status_InvalidTypeCodeObject:
    return "kInvalidTypeCodeObject";
  case tvm_rpc__rpc_status_InvalidTypeCodeNDArray:
    return "kInvalidTypeCodeNDArray";
  case tvm_rpc__rpc_status_InvalidDLTensorFieldStride:
    return "kInvalidDLTensorFieldStride";
  case tvm_rpc__rpc_status_InvalidDLTensorFieldByteOffset:
    return "kInvalidDLTensorFieldByteOffset";
  case tvm_rpc__rpc_status_UnknownTypeCode:
    return "kUnknownTypeCode";
  case tvm_rpc__rpc_status_UnknownRPCCode:
    return "kUnknownRPCCode";
  case tvm_rpc__rpc_status_RPCCodeNotSupported:
    return "RPCCodeNotSupported";
  case tvm_rpc__rpc_status_UnknownRPCSyscall:
    return "kUnknownRPCSyscall";
  case tvm_rpc__rpc_status_CheckError:
    return "kCheckError";
  case tvm_rpc__rpc_status_ReadError:
    return "kReadError";
  case tvm_rpc__rpc_status_WriteError:
    return "kWriteError";
  case tvm_rpc__rpc_status_AllocError:
    return "kAllocError";
  default:
    return "";
  }
}
