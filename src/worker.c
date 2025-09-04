#include "worker.h"
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

typedef struct worker_impl {
    worker_t*   w;
    SDL_Thread* th;
} worker_impl_t;

static int worker_thread_fn(void* userdata) {
    worker_impl_t* impl = (worker_impl_t*)userdata;
    worker_t* w = impl->w;

    if (!w->drv) return -1;
    if (w->drv->open(&w->dev) != FTDI_OK) return -2;

    // defaults (ถ้า driver รองรับ)
    ftdi_params_t p = { .baudrate = 115200, .data_bits = 8, .stop_bits = 1, .parity = 0, .latency_ms = 2 };
    w->drv->set_params(w->dev, &p);

    uint8_t buf[4096];
    while (w->running) {
        size_t nread = 0;
        int rc = w->drv->read(w->dev, buf, sizeof(buf), &nread, w->read_timeout_ms);
        if (rc == FTDI_OK && nread > 0) {
            rb_push(w->rxbuf, buf, nread);
            w->bytes_rx += nread;
        } else if (rc == FTDI_E_TIMEOUT) {
            /* nothing */
        } else if (rc != FTDI_OK) {
            break;
        }
        SDL_Delay(1);
    }

    if (w->dev) w->drv->close(w->dev);
    w->dev = NULL;
    return 0;
}

int worker_start(worker_t* w, const ftdi_vtbl_t* drv) {
    if (!w || !drv) return -1;
    memset(w, 0, sizeof(*w));
    w->drv = drv;
    w->rxbuf = rb_create(1<<20); // 1MB
    w->read_timeout_ms = 50;
    w->running = 1;

    worker_impl_t* impl = (worker_impl_t*)calloc(1, sizeof(*impl));
    impl->w = w;

    SDL_Thread* th = SDL_CreateThread(worker_thread_fn, "ftdi_worker", impl);
    if (!th) {
        w->running = 0;
        rb_destroy(w->rxbuf); w->rxbuf = NULL;
        free(impl);
        return -2;
    }

    impl->th = th;
    SDL_DetachThread(th); // ตัวอย่างง่าย ๆ; โปรดเปลี่ยนเป็น join ถ้าจะเก็บทุกรอยรั่ว
    return 0;
}

void worker_stop(worker_t* w) {
    if (!w) return;
    w->running = 0;
    SDL_Delay(60); // ปล่อยให้เธรดจบ
    if (w->rxbuf) { rb_destroy(w->rxbuf); w->rxbuf = NULL; }
}

int worker_send(worker_t* w, const uint8_t* data, size_t n) {
    if (!w || !w->drv || !w->dev) return -1;
    size_t nw = 0;
    int rc = w->drv->write(w->dev, data, n, &nw);
    if (rc == FTDI_OK) w->bytes_tx += nw;
    return rc;
}
