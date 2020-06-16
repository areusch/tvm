#include <iostream>
#include <unistd.h>

#include <inttypes.h>

#include <tvm/runtime/crt/logging.h>
#include <tvm/runtime/micro/micro_rpc_server.h>

extern "C" {

ssize_t utvm_write_func(void* context, const uint8_t* data, size_t num_bytes) {
  // fprintf(stderr, "sw\n");
  for (size_t i = 0; i < num_bytes; i++) {
    // fprintf(stderr, "w: %02x\n", data[i]);
  }
  ssize_t to_return = write(STDOUT_FILENO, data, num_bytes);
  fflush(stdout);
  fsync(STDOUT_FILENO);
  // fprintf(stderr, "WD\n");
  return to_return;
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
    // fprintf(stderr, "start read\n");
    int ret_code = read(STDIN_FILENO, &c, 1);
    if (ret_code < 0) {
      perror("utvm runtime: read failed");
      return 2;
    } else if (ret_code == 0) {
      fprintf(stderr, "utvm runtime: 0-length read, exiting!\n");
      return 2;
    }
    // fprintf(stderr, "read: %02x\n", c);
    if (utvm_rpc_server_receive_byte(rpc_server, c) != 1) {
      abort();
    }
    // fprintf(stderr, "SL\n");
    utvm_rpc_server_loop(rpc_server);
    // fprintf(stderr, "LD\n");
  }
  return 0;
}
