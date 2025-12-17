#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

typedef struct {
  uv_udp_t handle;
} server_ctx_t;

static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  (void)handle;
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

static void send_cb(uv_udp_send_t* req, int status) {
  (void)status;
  if (req->data) {
    free(req->data);
  }
  free(req);
}

static void recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf,
                    const struct sockaddr* addr, unsigned flags) {
  (void)flags;
  if (nread < 0 || addr == NULL) {
    free(buf->base);
    return;
  }
  uv_udp_send_t* send_req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
  uv_buf_t send_buf = uv_buf_init(buf->base, (unsigned)nread);
  send_req->data = send_buf.base;
  uv_udp_send(send_req, handle, &send_buf, 1, addr, send_cb);
}

int main(int argc, char** argv) {
  const unsigned short port =
      (argc > 1) ? (unsigned short)atoi(argv[1]) : 4242;

  uv_loop_t* loop = uv_default_loop();
  server_ctx_t ctx;

  uv_udp_init(loop, &ctx.handle);
  struct sockaddr_in addr;
  uv_ip4_addr("0.0.0.0", port, &addr);
  uv_udp_bind(&ctx.handle, (const struct sockaddr*)&addr, UV_UDP_REUSEADDR);

  printf("[libuv] Echo server listening on %u\n", port);
  uv_udp_recv_start(&ctx.handle, alloc_cb, recv_cb);
  uv_run(loop, UV_RUN_DEFAULT);
  uv_loop_close(loop);
  return 0;
}
