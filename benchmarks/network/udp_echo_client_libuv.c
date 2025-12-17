#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

typedef struct {
  uv_udp_t handle;
  uv_udp_send_t send_req;
  uv_buf_t send_buf;
  struct sockaddr_in server_addr;
  uint64_t sent;
  uint64_t received;
  uint64_t end_time_ms;
  size_t payload_size;
} client_ctx_t;

static uint64_t now_ms() {
  return uv_hrtime() / 1000000ULL;
}

static void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  (void)handle;
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

static void on_send(uv_udp_send_t* req, int status) {
  (void)status;
  (void)req;
}

static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf,
                    const struct sockaddr* addr, unsigned flags) {
  (void)flags;
  (void)addr;
  client_ctx_t* ctx = (client_ctx_t*)handle->data;
  if (nread > 0 && (size_t)nread == ctx->payload_size) {
    ctx->received++;
  }
  if (buf->base) free(buf->base);
}

static void idle_cb(uv_idle_t* idle) {
  client_ctx_t* ctx = (client_ctx_t*)idle->data;
  const uint64_t now = now_ms();
  if (now >= ctx->end_time_ms) {
    uv_idle_stop(idle);
    uv_stop(idle->loop);
    return;
  }
  uv_udp_send(&ctx->send_req, &ctx->handle, &ctx->send_buf, 1,
              (const struct sockaddr*)&ctx->server_addr, on_send);
  ctx->sent++;
}

int main(int argc, char** argv) {
  const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
  const int port = (argc > 2) ? atoi(argv[2]) : 4242;
  const int duration_sec = (argc > 3) ? atoi(argv[3]) : 30;
  const size_t payload_size = (argc > 4) ? (size_t)atoi(argv[4]) : 512;

  uv_loop_t* loop = uv_default_loop();
  client_ctx_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.payload_size = payload_size;

  uv_ip4_addr(host, port, &ctx.server_addr);
  uv_udp_init(loop, &ctx.handle);
  ctx.handle.data = &ctx;

  ctx.send_buf = uv_buf_init((char*)malloc(payload_size), payload_size);
  memset(ctx.send_buf.base, 'x', payload_size);

  ctx.end_time_ms = now_ms() + (uint64_t)duration_sec * 1000ull;

  uv_udp_recv_start(&ctx.handle, alloc_cb, on_recv);
  uv_idle_t idle;
  uv_idle_init(loop, &idle);
  idle.data = &ctx;
  uv_idle_start(&idle, idle_cb);

  uv_run(loop, UV_RUN_DEFAULT);

  uint64_t elapsed = (uint64_t)duration_sec * 1000ull;
  printf("[libuv] Duration %llu ms, sent %llu, received %llu, packets/s %.2f\n",
         (unsigned long long)elapsed, (unsigned long long)ctx.sent,
         (unsigned long long)ctx.received,
         (ctx.sent * 1000.0) / (double)elapsed);

  free(ctx.send_buf.base);
  uv_loop_close(loop);
  return 0;
}
