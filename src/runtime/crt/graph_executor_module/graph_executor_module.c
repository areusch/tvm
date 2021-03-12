/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

// LINT_C_FILE

/*!
 * \file graph_executor_module.c
 * \brief wrap graph_executor into a TVMModule for use with RPC.
 */

#include <tvm/runtime/crt/func_registry.h>
#include <tvm/runtime/crt/graph_executor.h>
#include <tvm/runtime/crt/graph_executor_module.h>
#include <tvm/runtime/crt/module.h>

#include "tvm/runtime/crt/internal/graph_executor/graph_executor.h"

typedef struct {
  TVMModule mod;
  TVMGraphExecutor* runtime;
} GraphExecutorModule;

static GraphExecutorModule graph_executor;

int32_t TVMGraphExecutorModule_Create(TVMValue* args, int* tcodes, int nargs, TVMValue* ret_values,
                                      int* ret_tcodes, void* resource_handle) {
  if (graph_executor.runtime != NULL) {
    return kTvmErrorGraphModuleAlreadyCreated;
  }

  if (nargs != 4) {
    return kTvmErrorFunctionCallNumArguments;
  }

  if (tcodes[0] != kTVMStr || tcodes[1] != kTVMModuleHandle || tcodes[2] != kTVMArgInt ||
      tcodes[3] != kTVMArgInt) {
    return kTvmErrorFunctionCallWrongArgType;
  }

  if (args[2].v_int64 != kDLCPU || args[3].v_int64 != 0) {
    return kTvmErrorGraphModuleBadContext;
  }

  DLDevice dev = {(DLDeviceType)args[2].v_int64, (int)args[3].v_int64};
  int ret_value =
      TVMGraphExecutor_Create(args[0].v_str, args[1].v_handle, &dev, &graph_executor.runtime);
  if (ret_value != 0) {
    return ret_value;
  }

  TVMModuleHandle out;
  ret_value = TVMModCreateFromCModule(&graph_executor.mod, &out);
  if (ret_value != 0) {
    ret_tcodes[0] = kTVMNullptr;
    TVMGraphExecutor_Release(&graph_executor.runtime);
    return ret_value;
  }

  ret_values[0].v_handle = out;
  ret_tcodes[0] = kTVMModuleHandle;
  return kTvmErrorNoError;
}

int32_t TVMGraphExecutorModule_GetInput(TVMValue* args, int* tcodes, int nargs,
                                        TVMValue* ret_values, int* ret_tcodes,
                                        void* resource_handle) {
  if (nargs != 1) {
    return kTvmErrorFunctionCallNumArguments;
  }

  if (tcodes[0] != kTVMStr) {
    return kTvmErrorFunctionCallWrongArgType;
  }

  int index = TVMGraphExecutor_GetInputIndex(graph_executor.runtime, args[0].v_str);
  if (index < 0) {
    return kTvmErrorGraphModuleNoSuchInput;
  }

  uint32_t eid = TVMGraphExecutor_GetEntryId(graph_executor.runtime,
                                             graph_executor.runtime->input_nodes[index], 0);
  ret_values[0].v_handle = (void*)&graph_executor.runtime->data_entry[eid].dl_tensor;
  ret_tcodes[0] = kTVMNDArrayHandle;
  return 0;
}

int32_t TVMGraphExecutorModule_GetNumInputs(TVMValue* args, int* tcodes, int nargs,
                                            TVMValue* ret_values, int* ret_tcodes,
                                            void* resource_handle) {
  if (nargs != 0) {
    return kTvmErrorFunctionCallNumArguments;
  }

  ret_values[0].v_int64 = TVMGraphExecutor_GetNumInputs();
  ret_tcodes[0] = kTVMArgInt;
  return 0;
}

int32_t TVMGraphExecutorModule_GetNumOutputs(TVMValue* args, int* tcodes, int nargs,
                                             TVMValue* ret_values, int* ret_tcodes,
                                             void* resource_handle) {
  if (nargs != 0) {
    return kTvmErrorFunctionCallNumArguments;
  }

  ret_values[0].v_int64 = TVMGraphExecutor_GetNumOutputs(graph_executor.runtime);
  ret_tcodes[0] = kTVMArgInt;
  return 0;
}

int32_t TVMGraphExecutorModule_GetOutput(TVMValue* args, int* tcodes, int nargs,
                                         TVMValue* ret_values, int* ret_tcodes,
                                         void* resource_handle) {
  if (nargs != 1) {
    return kTvmErrorFunctionCallNumArguments;
  }

  if (tcodes[0] != kTVMArgInt) {
    return kTvmErrorFunctionCallWrongArgType;
  }

  int output_index = args[0].v_int64;
  if (output_index < 0 || output_index > TVMGraphExecutor_GetNumOutputs(graph_executor.runtime)) {
    return kTvmErrorGraphModuleNoSuchInput;
  }

  uint32_t nid = graph_executor.runtime->outputs[output_index].node_id;
  uint32_t index = graph_executor.runtime->outputs[output_index].index;
  uint32_t eid = TVMGraphExecutor_GetEntryId(graph_executor.runtime, nid, index);

  ret_values[0].v_handle = (void*)&(graph_executor.runtime->data_entry[eid].dl_tensor);
  ret_tcodes[0] = kTVMNDArrayHandle;
  return 0;
}

int32_t TVMGraphExecutorModule_LoadParams(TVMValue* args, int* tcodes, int nargs,
                                          TVMValue* ret_values, int* ret_tcodes,
                                          void* resource_handle) {
  if (nargs != 1) {
    return kTvmErrorFunctionCallNumArguments;
  }

  if (tcodes[0] != kTVMBytes) {
    return kTvmErrorFunctionCallWrongArgType;
  }

  ret_tcodes[0] = kTVMNullptr;

  TVMByteArray* arr = (TVMByteArray*)args[0].v_handle;
  return TVMGraphExecutor_LoadParams(graph_executor.runtime, arr->data, arr->size);
}

int32_t TVMGraphExecutorModule_Run(TVMValue* args, int* tcodes, int nargs, TVMValue* ret_values,
                                   int* ret_tcodes, void* resource_handle) {
  if (nargs != 0) {
    return kTvmErrorFunctionCallNumArguments;
  }

  TVMGraphExecutor_Run(graph_executor.runtime);

  ret_tcodes[0] = kTVMNullptr;
  return 0;
}

int32_t TVMGraphExecutorModule_SetInput(TVMValue* args, int* tcodes, int nargs,
                                        TVMValue* ret_values, int* ret_tcodes,
                                        void* resource_handle) {
  if (nargs != 2) {
    return kTvmErrorFunctionCallNumArguments;
  }

  if (tcodes[0] != kTVMStr || tcodes[1] != kTVMDLTensorHandle) {
    return kTvmErrorFunctionCallWrongArgType;
  }

  TVMGraphExecutor_SetInput(graph_executor.runtime, args[0].v_str, (DLTensor*)args[1].v_handle);

  ret_tcodes[0] = kTVMNullptr;
  return 0;
}

int32_t TVMGraphExecutorModule_NotImplemented(TVMValue* args, int* tcodes, int nargs,
                                              TVMValue* ret_values, int* ret_tcodes,
                                              void* resource_handle) {
  return kTvmErrorFunctionCallNotImplemented;
}

static const TVMBackendPackedCFunc graph_executor_registry_funcs[] = {
    &TVMGraphExecutorModule_GetInput,      &TVMGraphExecutorModule_GetNumInputs,
    &TVMGraphExecutorModule_GetNumOutputs, &TVMGraphExecutorModule_GetOutput,
    &TVMGraphExecutorModule_LoadParams,    &TVMGraphExecutorModule_Run,
    &TVMGraphExecutorModule_SetInput,      &TVMGraphExecutorModule_NotImplemented,
};

static const TVMFuncRegistry graph_executor_registry = {
    "\x08get_input\0"
    "get_num_inputs\0"
    "get_num_outputs\0"
    "get_output\0"
    "load_params\0"
    "run\0"
    "set_input\0"
    "share_params\0",
    graph_executor_registry_funcs};

tvm_crt_error_t TVMGraphExecutorModule_Register() {
  graph_executor.mod.registry = &graph_executor_registry;
  graph_executor.runtime = NULL;

  return TVMFuncRegisterGlobal("tvm.graph_executor.create", &TVMGraphExecutorModule_Create, 0);
}
