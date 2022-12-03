// tvm target: c -keys=cpu 
#define TVM_EXPORTS
#include "tvm/runtime/c_runtime_api.h"
#include "tvm/runtime/c_backend_api.h"
#include <math.h>
static void* __tvm_set_device_packed = NULL;

#ifdef __cplusplus
extern "C" {
#endif
static const int8_t __attribute__((section(".rodata.tvm"), aligned(16))) fused_constant_0[400] = {
    -0x19, -0x07, -0x2e, +0x06, -0x77, +0x5c, +0x1c, -0x0e, 
    +0x35, +0x20, +0x28, +0x2a, -0x80, +0x4a, +0x02, -0x6e, 
    -0x5e, +0x26, +0x49, +0x02, +0x53, +0x29, +0x59, -0x68, 
    -0x6d, +0x1e, -0x6b, -0x70, +0x5a, +0x72, +0x0a, +0x34, 
    +0x64, -0x58, -0x08, +0x63, -0x7c, +0x25, +0x32, +0x4f, 
    -0x3d, -0x09, -0x2a, -0x59, +0x02, -0x2d, -0x35, -0x49, 
    +0x01, +0x2c, -0x6a, +0x3b, +0x06, -0x76, -0x3f, +0x0a, 
    -0x08, +0x45, +0x16, -0x20, +0x54, -0x32, +0x65, +0x2c, 
    +0x0f, +0x39, -0x1f, +0x75, +0x4d, -0x7c, +0x59, +0x3c, 
    -0x31, -0x46, -0x6c, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x40, -0x17, +0x02, -0x67, 
    -0x62, -0x03, +0x7a, -0x64, -0x11, -0x63, -0x1f, +0x17, 
    +0x74, -0x0e, +0x6d, -0x7b, +0x68, +0x10, +0x4a, -0x79, 
    +0x1d, +0x01, -0x3e, +0x46, +0x7d, -0x69, -0x12, +0x5f, 
    +0x3f, -0x75, +0x79, -0x53, -0x72, -0x18, -0x28, +0x29, 
    +0x2f, +0x2c, +0x44, -0x11, -0x13, +0x1c, +0x18, -0x6b, 
    -0x6c, -0x5a, -0x20, -0x13, +0x32, +0x78, -0x47, -0x04, 
    +0x58, -0x65, -0x75, +0x6c, +0x52, +0x46, +0x76, +0x55, 
    +0x6e, +0x7c, +0x76, -0x3d, -0x29, +0x4a, +0x24, -0x48, 
    +0x1f, -0x03, +0x7d, -0x70, +0x57, +0x35, +0x46, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x06, -0x72, -0x0e, +0x28, -0x61, -0x7f, +0x0d, +0x03, 
    +0x2b, -0x74, -0x2b, -0x80, -0x79, -0x72, +0x4f, -0x63, 
    -0x11, +0x5e, -0x6b, +0x14, +0x6a, +0x57, -0x5d, +0x74, 
    +0x68, +0x2d, +0x6d, -0x33, -0x36, +0x2d, +0x2c, +0x59, 
    +0x2d, -0x45, +0x64, +0x5f, +0x10, -0x2e, -0x06, -0x6e, 
    +0x1a, -0x34, +0x68, +0x02, -0x60, -0x35, -0x29, +0x3f, 
    +0x3b, +0x11, +0x5d, -0x0a, +0x05, +0x1f, +0x32, -0x10, 
    -0x20, +0x3e, +0x62, -0x69, -0x6f, -0x48, -0x5a, -0x3a, 
    +0x32, +0x18, -0x26, -0x12, -0x2e, +0x52, -0x3a, -0x34, 
    -0x11, -0x76, +0x34, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, 
    +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00, +0x00
};
#ifdef __cplusplus
}  // extern "C"
#endif
static void* tvmgen_default_fused_nn_conv2d_kernel0_packed = NULL;
static void* tvmgen_default_fused_nn_conv2d_kernel1_packed = NULL;
static void* tvmgen_default_fused_nn_conv2d_kernel2_packed = NULL;
static void* tvmgen_default_fused_nn_pad_kernel0_packed = NULL;
static void* tvmgen_default_fused_strided_slice_kernel0_packed = NULL;
static void* tvm_runtime_aot_call_device_api_packed = NULL;
static void* tvmgen_default_fused_nn_pad_packed = NULL;
static void* tvmgen_default_fused_nn_conv2d_packed = NULL;
static void* tvmgen_default_fused_strided_slice_packed = NULL;
void* __tvm_module_ctx = NULL;
#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t tvmgen_default_fused_nn_conv2d(void* args, int32_t* arg_type_ids, int32_t num_args, void* out_ret_value, int32_t* out_ret_tcode, void* resource_handle) {
  TVMValue stack[8];
  void* stack_tcode = stack;
  TVMValue stack_1[16];
  void* stack_value = stack_1;
  void* arg_p0 = (((TVMValue*)args)[0].v_handle);
  int32_t arg_p0_code = arg_type_ids[0];
  void* arg_output_unpack = (((TVMValue*)args)[1].v_handle);
  int32_t arg_output_unpack_code = arg_type_ids[1];
  void* p0 = (((DLTensor*)arg_p0)[0].data);
  void* arg_p0_shape = (((DLTensor*)arg_p0)[0].shape);
  void* arg_p0_strides = (((DLTensor*)arg_p0)[0].strides);
  int32_t dev_id = (((DLTensor*)arg_p0)[0].device.device_id);
  void* output_unpack = (((DLTensor*)arg_output_unpack)[0].data);
  void* arg_output_unpack_shape = (((DLTensor*)arg_output_unpack)[0].shape);
  void* arg_output_unpack_strides = (((DLTensor*)arg_output_unpack)[0].strides);
  if (!(arg_p0_strides == NULL)) {
  }
  if (!(arg_output_unpack_strides == NULL)) {
  }
  (((TVMValue*)stack_value)[0].v_int64) = ((int64_t)2);
  ((int32_t*)stack_tcode)[0] = 0;
  (((TVMValue*)stack_value)[1].v_int64) = ((int64_t)dev_id);
  ((int32_t*)stack_tcode)[1] = 0;
  if (__tvm_set_device_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "__tvm_set_device", &__tvm_set_device_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val;
  int ret_type_code;
  if (TVMFuncCall(__tvm_set_device_packed, (TVMValue*) stack_value, (int*) stack_tcode, 2, &ret_val, &ret_type_code) != 0) {
    return -1;
  }
  ret_val.v_int64;
  void* packed_data = TVMBackendAllocWorkspace(2, dev_id, (uint64_t)16384, 0, 8);
  if (packed_data == NULL) {
    return -1;
  }
  void* packed_kernel = TVMBackendAllocWorkspace(2, dev_id, (uint64_t)400, 0, 8);
  if (packed_kernel == NULL) {
    return -1;
  }
  (((TVMValue*)stack_value)[0].v_handle) = packed_data;
  ((int32_t*)stack_tcode)[0] = 3;
  (((TVMValue*)stack_value)[1].v_handle) = p0;
  ((int32_t*)stack_tcode)[1] = 3;
  (((TVMValue*)stack_value)[2].v_int64) = ((int64_t)16);
  ((int32_t*)stack_tcode)[2] = 0;
  (((TVMValue*)stack_value)[3].v_int64) = ((int64_t)1024);
  ((int32_t*)stack_tcode)[3] = 0;
  if (tvmgen_default_fused_nn_conv2d_kernel0_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvmgen_default_fused_nn_conv2d_kernel0", &tvmgen_default_fused_nn_conv2d_kernel0_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_1;
  int ret_type_code_1;
  if (TVMFuncCall(tvmgen_default_fused_nn_conv2d_kernel0_packed, (TVMValue*) stack_value, (int*) stack_tcode, 4, &ret_val_1, &ret_type_code_1) != 0) {
    return -1;
  }
  ret_val_1.v_int64;
  (((TVMValue*)stack_value)[0].v_handle) = packed_kernel;
  ((int32_t*)stack_tcode)[0] = 3;
  (((TVMValue*)stack_value)[1].v_handle) = fused_constant_0;
  ((int32_t*)stack_tcode)[1] = 3;
  (((TVMValue*)stack_value)[2].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[2] = 0;
  (((TVMValue*)stack_value)[3].v_int64) = ((int64_t)400);
  ((int32_t*)stack_tcode)[3] = 0;
  if (tvmgen_default_fused_nn_conv2d_kernel1_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvmgen_default_fused_nn_conv2d_kernel1", &tvmgen_default_fused_nn_conv2d_kernel1_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_2;
  int ret_type_code_2;
  if (TVMFuncCall(tvmgen_default_fused_nn_conv2d_kernel1_packed, (TVMValue*) stack_value, (int*) stack_tcode, 4, &ret_val_2, &ret_type_code_2) != 0) {
    return -1;
  }
  ret_val_2.v_int64;
  (((TVMValue*)stack_value)[0].v_handle) = packed_data;
  ((int32_t*)stack_tcode)[0] = 3;
  (((TVMValue*)stack_value)[1].v_handle) = packed_kernel;
  ((int32_t*)stack_tcode)[1] = 3;
  (((TVMValue*)stack_value)[2].v_handle) = output_unpack;
  ((int32_t*)stack_tcode)[2] = 3;
  (((TVMValue*)stack_value)[3].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[3] = 0;
  (((TVMValue*)stack_value)[4].v_int64) = ((int64_t)4);
  ((int32_t*)stack_tcode)[4] = 0;
  (((TVMValue*)stack_value)[5].v_int64) = ((int64_t)4096);
  ((int32_t*)stack_tcode)[5] = 0;
  (((TVMValue*)stack_value)[6].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[6] = 0;
  (((TVMValue*)stack_value)[7].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[7] = 0;
  (((TVMValue*)stack_value)[8].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[8] = 0;
  (((TVMValue*)stack_value)[9].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[9] = 0;
  (((TVMValue*)stack_value)[10].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[10] = 0;
  (((TVMValue*)stack_value)[11].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[11] = 0;
  (((TVMValue*)stack_value)[12].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[12] = 0;
  (((TVMValue*)stack_value)[13].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[13] = 0;
  (((TVMValue*)stack_value)[14].v_int64) = ((int64_t)1);
  ((int32_t*)stack_tcode)[14] = 0;
  if (tvmgen_default_fused_nn_conv2d_kernel2_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvmgen_default_fused_nn_conv2d_kernel2", &tvmgen_default_fused_nn_conv2d_kernel2_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_3;
  int ret_type_code_3;
  if (TVMFuncCall(tvmgen_default_fused_nn_conv2d_kernel2_packed, (TVMValue*) stack_value, (int*) stack_tcode, 15, &ret_val_3, &ret_type_code_3) != 0) {
    return -1;
  }
  ret_val_3.v_int64;
  if (TVMBackendFreeWorkspace(2, dev_id, packed_kernel) != 0) {
    return -1;
  }
  if (TVMBackendFreeWorkspace(2, dev_id, packed_data) != 0) {
    return -1;
  }
  return 0;
}

#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t tvmgen_default_fused_nn_pad(void* args, int32_t* arg_type_ids, int32_t num_args, void* out_ret_value, int32_t* out_ret_tcode, void* resource_handle) {
  TVMValue stack[3];
  void* stack_tcode = stack;
  TVMValue stack_1[5];
  void* stack_value = stack_1;
  void* arg_p0 = (((TVMValue*)args)[0].v_handle);
  int32_t arg_p0_code = arg_type_ids[0];
  void* arg_T_pad = (((TVMValue*)args)[1].v_handle);
  int32_t arg_T_pad_code = arg_type_ids[1];
  void* p0 = (((DLTensor*)arg_p0)[0].data);
  void* arg_p0_shape = (((DLTensor*)arg_p0)[0].shape);
  void* arg_p0_strides = (((DLTensor*)arg_p0)[0].strides);
  int32_t dev_id = (((DLTensor*)arg_p0)[0].device.device_id);
  void* T_pad = (((DLTensor*)arg_T_pad)[0].data);
  void* arg_T_pad_shape = (((DLTensor*)arg_T_pad)[0].shape);
  void* arg_T_pad_strides = (((DLTensor*)arg_T_pad)[0].strides);
  if (!(arg_p0_strides == NULL)) {
  }
  if (!(arg_T_pad_strides == NULL)) {
  }
  (((TVMValue*)stack_value)[0].v_int64) = ((int64_t)2);
  ((int32_t*)stack_tcode)[0] = 0;
  (((TVMValue*)stack_value)[1].v_int64) = ((int64_t)dev_id);
  ((int32_t*)stack_tcode)[1] = 0;
  if (__tvm_set_device_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "__tvm_set_device", &__tvm_set_device_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val;
  int ret_type_code;
  if (TVMFuncCall(__tvm_set_device_packed, (TVMValue*) stack_value, (int*) stack_tcode, 2, &ret_val, &ret_type_code) != 0) {
    return -1;
  }
  ret_val.v_int64;
  (((TVMValue*)stack_value)[0].v_handle) = T_pad;
  ((int32_t*)stack_tcode)[0] = 3;
  (((TVMValue*)stack_value)[1].v_handle) = p0;
  ((int32_t*)stack_tcode)[1] = 3;
  (((TVMValue*)stack_value)[2].v_int64) = ((int64_t)16);
  ((int32_t*)stack_tcode)[2] = 0;
  (((TVMValue*)stack_value)[3].v_int64) = ((int64_t)1024);
  ((int32_t*)stack_tcode)[3] = 0;
  if (tvmgen_default_fused_nn_pad_kernel0_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvmgen_default_fused_nn_pad_kernel0", &tvmgen_default_fused_nn_pad_kernel0_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_1;
  int ret_type_code_1;
  if (TVMFuncCall(tvmgen_default_fused_nn_pad_kernel0_packed, (TVMValue*) stack_value, (int*) stack_tcode, 4, &ret_val_1, &ret_type_code_1) != 0) {
    return -1;
  }
  ret_val_1.v_int64;
  return 0;
}

#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t tvmgen_default_fused_strided_slice(void* args, int32_t* arg_type_ids, int32_t num_args, void* out_ret_value, int32_t* out_ret_tcode, void* resource_handle) {
  TVMValue stack[3];
  void* stack_tcode = stack;
  TVMValue stack_1[5];
  void* stack_value = stack_1;
  void* arg_p0 = (((TVMValue*)args)[0].v_handle);
  int32_t arg_p0_code = arg_type_ids[0];
  void* arg_T_strided_slice = (((TVMValue*)args)[1].v_handle);
  int32_t arg_T_strided_slice_code = arg_type_ids[1];
  void* p0 = (((DLTensor*)arg_p0)[0].data);
  void* arg_p0_shape = (((DLTensor*)arg_p0)[0].shape);
  void* arg_p0_strides = (((DLTensor*)arg_p0)[0].strides);
  int32_t dev_id = (((DLTensor*)arg_p0)[0].device.device_id);
  void* T_strided_slice = (((DLTensor*)arg_T_strided_slice)[0].data);
  void* arg_T_strided_slice_shape = (((DLTensor*)arg_T_strided_slice)[0].shape);
  void* arg_T_strided_slice_strides = (((DLTensor*)arg_T_strided_slice)[0].strides);
  if (!(arg_p0_strides == NULL)) {
  }
  if (!(arg_T_strided_slice_strides == NULL)) {
  }
  (((TVMValue*)stack_value)[0].v_int64) = ((int64_t)2);
  ((int32_t*)stack_tcode)[0] = 0;
  (((TVMValue*)stack_value)[1].v_int64) = ((int64_t)dev_id);
  ((int32_t*)stack_tcode)[1] = 0;
  if (__tvm_set_device_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "__tvm_set_device", &__tvm_set_device_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val;
  int ret_type_code;
  if (TVMFuncCall(__tvm_set_device_packed, (TVMValue*) stack_value, (int*) stack_tcode, 2, &ret_val, &ret_type_code) != 0) {
    return -1;
  }
  ret_val.v_int64;
  (((TVMValue*)stack_value)[0].v_handle) = T_strided_slice;
  ((int32_t*)stack_tcode)[0] = 3;
  (((TVMValue*)stack_value)[1].v_handle) = p0;
  ((int32_t*)stack_tcode)[1] = 3;
  (((TVMValue*)stack_value)[2].v_int64) = ((int64_t)12);
  ((int32_t*)stack_tcode)[2] = 0;
  (((TVMValue*)stack_value)[3].v_int64) = ((int64_t)1024);
  ((int32_t*)stack_tcode)[3] = 0;
  if (tvmgen_default_fused_strided_slice_kernel0_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvmgen_default_fused_strided_slice_kernel0", &tvmgen_default_fused_strided_slice_kernel0_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_1;
  int ret_type_code_1;
  if (TVMFuncCall(tvmgen_default_fused_strided_slice_kernel0_packed, (TVMValue*) stack_value, (int*) stack_tcode, 4, &ret_val_1, &ret_type_code_1) != 0) {
    return -1;
  }
  ret_val_1.v_int64;
  return 0;
}

#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t tvmgen_default___tvm_main__(void* args, int32_t* arg_type_ids, int32_t num_args, void* out_ret_value, int32_t* out_ret_tcode, void* resource_handle) {
  TVMValue stack[4];
  void* stack_tcode = stack;
  TVMValue stack_1[8];
  void* stack_value = stack_1;
  TVMValue stack_2[12];
  void* stack_array = stack_2;
  TVMValue stack_3[8];
  void* stack_shape = stack_3;
  void* arg_data = (((TVMValue*)args)[0].v_handle);
  int32_t arg_data_code = arg_type_ids[0];
  void* arg_output = (((TVMValue*)args)[1].v_handle);
  int32_t arg_output_code = arg_type_ids[1];
  void* data_buffer_var = (((DLTensor*)arg_data)[0].data);
  void* arg_data_shape = (((DLTensor*)arg_data)[0].shape);
  void* arg_data_strides = (((DLTensor*)arg_data)[0].strides);
  int32_t dev_id = (((DLTensor*)arg_data)[0].device.device_id);
  void* output_buffer_var = (((DLTensor*)arg_output)[0].data);
  void* arg_output_shape = (((DLTensor*)arg_output)[0].shape);
  void* arg_output_strides = (((DLTensor*)arg_output)[0].strides);
  if (!(arg_data_strides == NULL)) {
  }
  if (!(arg_output_strides == NULL)) {
  }
  (((TVMValue*)stack_value)[0].v_int64) = (int64_t)0;
  ((int32_t*)stack_tcode)[0] = 0;
  (((TVMValue*)stack_value)[1].v_int64) = (int64_t)0;
  ((int32_t*)stack_tcode)[1] = 0;
  (((TVMValue*)stack_value)[2].v_int64) = (int64_t)65536;
  ((int32_t*)stack_tcode)[2] = 0;
  (((TVMValue*)stack_value)[3].v_int64) = (int64_t)128;
  ((int32_t*)stack_tcode)[3] = 0;
  (((TVMValue*)stack_value)[4].v_int64) = (int64_t)0;
  ((int32_t*)stack_tcode)[4] = 0;
  (((TVMValue*)stack_value)[5].v_int64) = (int64_t)8;
  ((int32_t*)stack_tcode)[5] = 0;
  (((TVMValue*)stack_value)[6].v_int64) = (int64_t)1;
  ((int32_t*)stack_tcode)[6] = 0;
  if (tvm_runtime_aot_call_device_api_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvm.runtime.aot.call_device_api", &tvm_runtime_aot_call_device_api_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val;
  int ret_type_code;
  if (TVMFuncCall(tvm_runtime_aot_call_device_api_packed, (TVMValue*) stack_value, (int*) stack_tcode, 7, &ret_val, &ret_type_code) != 0) {
    return -1;
  }
  void* sid_2 = ret_val.v_handle;
  (((TVMValue*)stack_value)[0].v_int64) = (int64_t)0;
  ((int32_t*)stack_tcode)[0] = 0;
  (((TVMValue*)stack_value)[1].v_int64) = (int64_t)0;
  ((int32_t*)stack_tcode)[1] = 0;
  (((TVMValue*)stack_value)[2].v_int64) = (int64_t)16384;
  ((int32_t*)stack_tcode)[2] = 0;
  (((TVMValue*)stack_value)[3].v_int64) = (int64_t)128;
  ((int32_t*)stack_tcode)[3] = 0;
  (((TVMValue*)stack_value)[4].v_int64) = (int64_t)0;
  ((int32_t*)stack_tcode)[4] = 0;
  (((TVMValue*)stack_value)[5].v_int64) = (int64_t)8;
  ((int32_t*)stack_tcode)[5] = 0;
  (((TVMValue*)stack_value)[6].v_int64) = (int64_t)1;
  ((int32_t*)stack_tcode)[6] = 0;
  if (tvm_runtime_aot_call_device_api_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvm.runtime.aot.call_device_api", &tvm_runtime_aot_call_device_api_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_1;
  int ret_type_code_1;
  if (TVMFuncCall(tvm_runtime_aot_call_device_api_packed, (TVMValue*) stack_value, (int*) stack_tcode, 7, &ret_val_1, &ret_type_code_1) != 0) {
    return -1;
  }
  void* sid_1 = ret_val_1.v_handle;
  ((int64_t*)stack_shape)[0] = (int64_t)1;
  ((int64_t*)stack_shape)[1] = (int64_t)3;
  ((int64_t*)stack_shape)[2] = (int64_t)64;
  ((int64_t*)stack_shape)[3] = (int64_t)64;
  (((DLTensor*)stack_array)[0].data) = data_buffer_var;
  (((DLTensor*)stack_array)[0].shape) = (&(((int64_t*)stack_shape)[0]));
    uint64_t __1 = (uint64_t)0;
  (((DLTensor*)stack_array)[0].strides) = (int64_t*)(*(void* *)(&(__1)));
  (((DLTensor*)stack_array)[0].ndim) = (uint32_t)4;
  (((DLTensor*)stack_array)[0].dtype.code) = (uint8_t)0;
  (((DLTensor*)stack_array)[0].dtype.bits) = (uint8_t)8;
  (((DLTensor*)stack_array)[0].dtype.lanes) = (uint16_t)1;
  (((DLTensor*)stack_array)[0].byte_offset) = (uint64_t)0;
  (((DLTensor*)stack_array)[0].device.device_id) = dev_id;
  (((DLTensor*)stack_array)[0].device.device_type) = (DLDeviceType)1;
  ((int64_t*)stack_shape)[4] = (int64_t)1;
  ((int64_t*)stack_shape)[5] = (int64_t)4;
  ((int64_t*)stack_shape)[6] = (int64_t)64;
  ((int64_t*)stack_shape)[7] = (int64_t)64;
  (((DLTensor*)stack_array)[1].data) = sid_1;
  (((DLTensor*)stack_array)[1].shape) = (&(((int64_t*)stack_shape)[4]));
    uint64_t __2 = (uint64_t)0;
  (((DLTensor*)stack_array)[1].strides) = (int64_t*)(*(void* *)(&(__2)));
  (((DLTensor*)stack_array)[1].ndim) = (uint32_t)4;
  (((DLTensor*)stack_array)[1].dtype.code) = (uint8_t)0;
  (((DLTensor*)stack_array)[1].dtype.bits) = (uint8_t)8;
  (((DLTensor*)stack_array)[1].dtype.lanes) = (uint16_t)1;
  (((DLTensor*)stack_array)[1].byte_offset) = (uint64_t)0;
  (((DLTensor*)stack_array)[1].device.device_id) = dev_id;
  (((DLTensor*)stack_array)[1].device.device_type) = (DLDeviceType)1;
  (((TVMValue*)stack_value)[0].v_handle) = (((DLTensor*)stack_array) + 0);
  ((int32_t*)stack_tcode)[0] = 7;
  (((TVMValue*)stack_value)[1].v_handle) = (((DLTensor*)stack_array) + 1);
  ((int32_t*)stack_tcode)[1] = 7;
  if (tvmgen_default_fused_nn_pad_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvmgen_default_fused_nn_pad", &tvmgen_default_fused_nn_pad_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_2;
  int ret_type_code_2;
  if (TVMFuncCall(tvmgen_default_fused_nn_pad_packed, (TVMValue*) stack_value, (int*) stack_tcode, 2, &ret_val_2, &ret_type_code_2) != 0) {
    return -1;
  }
  ret_val_2.v_int64;
  ((int64_t*)stack_shape)[0] = (int64_t)1;
  ((int64_t*)stack_shape)[1] = (int64_t)4;
  ((int64_t*)stack_shape)[2] = (int64_t)64;
  ((int64_t*)stack_shape)[3] = (int64_t)64;
  (((DLTensor*)stack_array)[0].data) = sid_1;
  (((DLTensor*)stack_array)[0].shape) = (&(((int64_t*)stack_shape)[0]));
    uint64_t __3 = (uint64_t)0;
  (((DLTensor*)stack_array)[0].strides) = (int64_t*)(*(void* *)(&(__3)));
  (((DLTensor*)stack_array)[0].ndim) = (uint32_t)4;
  (((DLTensor*)stack_array)[0].dtype.code) = (uint8_t)0;
  (((DLTensor*)stack_array)[0].dtype.bits) = (uint8_t)8;
  (((DLTensor*)stack_array)[0].dtype.lanes) = (uint16_t)1;
  (((DLTensor*)stack_array)[0].byte_offset) = (uint64_t)0;
  (((DLTensor*)stack_array)[0].device.device_id) = dev_id;
  (((DLTensor*)stack_array)[0].device.device_type) = (DLDeviceType)1;
  ((int64_t*)stack_shape)[4] = (int64_t)1;
  ((int64_t*)stack_shape)[5] = (int64_t)4;
  ((int64_t*)stack_shape)[6] = (int64_t)64;
  ((int64_t*)stack_shape)[7] = (int64_t)64;
  (((DLTensor*)stack_array)[1].data) = sid_2;
  (((DLTensor*)stack_array)[1].shape) = (&(((int64_t*)stack_shape)[4]));
    uint64_t __4 = (uint64_t)0;
  (((DLTensor*)stack_array)[1].strides) = (int64_t*)(*(void* *)(&(__4)));
  (((DLTensor*)stack_array)[1].ndim) = (uint32_t)4;
  (((DLTensor*)stack_array)[1].dtype.code) = (uint8_t)0;
  (((DLTensor*)stack_array)[1].dtype.bits) = (uint8_t)32;
  (((DLTensor*)stack_array)[1].dtype.lanes) = (uint16_t)1;
  (((DLTensor*)stack_array)[1].byte_offset) = (uint64_t)0;
  (((DLTensor*)stack_array)[1].device.device_id) = dev_id;
  (((DLTensor*)stack_array)[1].device.device_type) = (DLDeviceType)1;
  (((TVMValue*)stack_value)[0].v_handle) = (((DLTensor*)stack_array) + 0);
  ((int32_t*)stack_tcode)[0] = 7;
  (((TVMValue*)stack_value)[1].v_handle) = (((DLTensor*)stack_array) + 1);
  ((int32_t*)stack_tcode)[1] = 7;
  if (tvmgen_default_fused_nn_conv2d_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvmgen_default_fused_nn_conv2d", &tvmgen_default_fused_nn_conv2d_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_3;
  int ret_type_code_3;
  if (TVMFuncCall(tvmgen_default_fused_nn_conv2d_packed, (TVMValue*) stack_value, (int*) stack_tcode, 2, &ret_val_3, &ret_type_code_3) != 0) {
    return -1;
  }
  ret_val_3.v_int64;
  ((int64_t*)stack_shape)[0] = (int64_t)1;
  ((int64_t*)stack_shape)[1] = (int64_t)4;
  ((int64_t*)stack_shape)[2] = (int64_t)64;
  ((int64_t*)stack_shape)[3] = (int64_t)64;
  (((DLTensor*)stack_array)[0].data) = sid_2;
  (((DLTensor*)stack_array)[0].shape) = (&(((int64_t*)stack_shape)[0]));
    uint64_t __5 = (uint64_t)0;
  (((DLTensor*)stack_array)[0].strides) = (int64_t*)(*(void* *)(&(__5)));
  (((DLTensor*)stack_array)[0].ndim) = (uint32_t)4;
  (((DLTensor*)stack_array)[0].dtype.code) = (uint8_t)0;
  (((DLTensor*)stack_array)[0].dtype.bits) = (uint8_t)32;
  (((DLTensor*)stack_array)[0].dtype.lanes) = (uint16_t)1;
  (((DLTensor*)stack_array)[0].byte_offset) = (uint64_t)0;
  (((DLTensor*)stack_array)[0].device.device_id) = dev_id;
  (((DLTensor*)stack_array)[0].device.device_type) = (DLDeviceType)1;
  ((int64_t*)stack_shape)[4] = (int64_t)1;
  ((int64_t*)stack_shape)[5] = (int64_t)3;
  ((int64_t*)stack_shape)[6] = (int64_t)64;
  ((int64_t*)stack_shape)[7] = (int64_t)64;
  (((DLTensor*)stack_array)[1].data) = output_buffer_var;
  (((DLTensor*)stack_array)[1].shape) = (&(((int64_t*)stack_shape)[4]));
    uint64_t __6 = (uint64_t)0;
  (((DLTensor*)stack_array)[1].strides) = (int64_t*)(*(void* *)(&(__6)));
  (((DLTensor*)stack_array)[1].ndim) = (uint32_t)4;
  (((DLTensor*)stack_array)[1].dtype.code) = (uint8_t)0;
  (((DLTensor*)stack_array)[1].dtype.bits) = (uint8_t)32;
  (((DLTensor*)stack_array)[1].dtype.lanes) = (uint16_t)1;
  (((DLTensor*)stack_array)[1].byte_offset) = (uint64_t)0;
  (((DLTensor*)stack_array)[1].device.device_id) = dev_id;
  (((DLTensor*)stack_array)[1].device.device_type) = (DLDeviceType)1;
  (((TVMValue*)stack_value)[0].v_handle) = (((DLTensor*)stack_array) + 0);
  ((int32_t*)stack_tcode)[0] = 7;
  (((TVMValue*)stack_value)[1].v_handle) = (((DLTensor*)stack_array) + 1);
  ((int32_t*)stack_tcode)[1] = 7;
  if (tvmgen_default_fused_strided_slice_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvmgen_default_fused_strided_slice", &tvmgen_default_fused_strided_slice_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_4;
  int ret_type_code_4;
  if (TVMFuncCall(tvmgen_default_fused_strided_slice_packed, (TVMValue*) stack_value, (int*) stack_tcode, 2, &ret_val_4, &ret_type_code_4) != 0) {
    return -1;
  }
  ret_val_4.v_int64;
  (((TVMValue*)stack_value)[0].v_int64) = (int64_t)0;
  ((int32_t*)stack_tcode)[0] = 0;
  (((TVMValue*)stack_value)[1].v_int64) = (int64_t)1;
  ((int32_t*)stack_tcode)[1] = 0;
  (((TVMValue*)stack_value)[2].v_handle) = sid_1;
  ((int32_t*)stack_tcode)[2] = 3;
  if (tvm_runtime_aot_call_device_api_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvm.runtime.aot.call_device_api", &tvm_runtime_aot_call_device_api_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_5;
  int ret_type_code_5;
  if (TVMFuncCall(tvm_runtime_aot_call_device_api_packed, (TVMValue*) stack_value, (int*) stack_tcode, 3, &ret_val_5, &ret_type_code_5) != 0) {
    return -1;
  }
  ret_val_5.v_int64;
  (((TVMValue*)stack_value)[0].v_int64) = (int64_t)0;
  ((int32_t*)stack_tcode)[0] = 0;
  (((TVMValue*)stack_value)[1].v_int64) = (int64_t)1;
  ((int32_t*)stack_tcode)[1] = 0;
  (((TVMValue*)stack_value)[2].v_handle) = sid_2;
  ((int32_t*)stack_tcode)[2] = 3;
  if (tvm_runtime_aot_call_device_api_packed == NULL) {
    if (TVMBackendGetFuncFromEnv(__tvm_module_ctx, "tvm.runtime.aot.call_device_api", &tvm_runtime_aot_call_device_api_packed) != 0) {
      return -1;
    }
  }
  TVMValue ret_val_6;
  int ret_type_code_6;
  if (TVMFuncCall(tvm_runtime_aot_call_device_api_packed, (TVMValue*) stack_value, (int*) stack_tcode, 3, &ret_val_6, &ret_type_code_6) != 0) {
    return -1;
  }
  ret_val_6.v_int64;
  return 0;
}

