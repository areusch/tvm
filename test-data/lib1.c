#include <inttypes.h>
#include <tvm/runtime/metadata_types.h>
#include <tvm/runtime/c_runtime_api.h>
static const int64_t kTvmgenMetadata_inputs_0_shape[4] = {
1L, 
3L, 
64L, 
64L};
static const struct TVMTensorInfo kTvmgenMetadata_inputs[1] = {
{
"data" /* name*/, 
kTvmgenMetadata_inputs_0_shape, 
4L /* num_shape*/, 
{0, 8, 1} /* dtype*/}
};
static const int64_t kTvmgenMetadata_outputs_0_shape[4] = {
1L, 
3L, 
64L, 
64L};
static const struct TVMTensorInfo kTvmgenMetadata_outputs[1] = {
{
"output0" /* name*/, 
kTvmgenMetadata_outputs_0_shape, 
4L /* num_shape*/, 
{0, 32, 1} /* dtype*/}
};
static const struct TVMTensorInfo kTvmgenMetadata_workspace_pools[0] = {
};
static const struct TVMConstantInfo kTvmgenMetadata_constant_pools[0] = {
};
static const struct TVMMetadata kTvmgenMetadata[1] = {
{
1L /* version*/, 
kTvmgenMetadata_inputs, 
1L /* num_inputs*/, 
kTvmgenMetadata_outputs, 
1L /* num_outputs*/, 
kTvmgenMetadata_workspace_pools, 
0L /* num_workspace_pools*/, 
kTvmgenMetadata_constant_pools, 
0L /* num_constant_pools*/, 
"tvmgen_default" /* mod_name*/}
};
#ifdef __cplusplus
extern "C"
#endif
TVM_DLL int32_t get_c_metadata(TVMValue* arg_values, int* arg_tcodes, int num_args, TVMValue* ret_values, int* ret_tcodes, void* resource_handle) {
    ret_values[0].v_handle = (void*) &kTvmgenMetadata;
    ret_tcodes[0] = kTVMOpaqueHandle;
    return 0;
};
