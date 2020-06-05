#include <iostream>
#include <unistd.h>

#include <inttypes.h>

#include <tvm/runtime/micro/micro_rpc_server.h>

extern "C" {

ssize_t utvm_write_func(void* context, const uint8_t* data, size_t num_bytes) {
  return write(1, data, num_bytes);
}

void TVMPlatformAbort(int exit_code) {
  std::cerr << "TVM Abort:" << exit_code << std::endl;
  throw "Aborted";
}
}

uint8_t memory[64 * 1024];

int main(int argc, char** argv) {
  utvm_rpc_server_t rpc_server = utvm_rpc_server_init(memory, sizeof(memory), 8, &utvm_write_func, nullptr);

  setbuf(stdin, NULL);
  setbuf(stdout, NULL);

  for (;;) {
    uint8_t c;
    read(STDIN_FILENO, &c, 1);
    if (utvm_rpc_server_receive_data(rpc_server, &c, 1) != 1) {
      abort();
    }
    utvm_rpc_server_loop(rpc_server);
  }
  return 0;
}
