#ifndef WORKER_H
#define WORKER_H

#include <stdint.h>
#include <stddef.h>
#include "ftdi.h"
#include "ringbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct worker {
    const ftdi_vtbl_t* drv;
    ftdi_handle_t*     dev;
    ringbuffer_t*      rxbuf;
    volatile int       running;   // ใช้ volatile flag แทน C11 atomics
    uint64_t           bytes_rx;
    uint64_t           bytes_tx;
    uint32_t           read_timeout_ms;
} worker_t;

int  worker_start(worker_t* w, const ftdi_vtbl_t* drv);
void worker_stop(worker_t* w);
int  worker_send(worker_t* w, const uint8_t* data, size_t n);

#ifdef __cplusplus
}
#endif

#endif // WORKER_H
