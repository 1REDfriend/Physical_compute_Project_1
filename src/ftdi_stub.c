#include "ftdi.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct ftdi_handle {
    int dummy;
    unsigned counter;
};

static int stub_open(ftdi_handle_t** out) {
    if (!out) return FTDI_E_GENERIC;
    *out = (ftdi_handle_t*)calloc(1, sizeof(ftdi_handle_t));
    return *out ? FTDI_OK : FTDI_E_GENERIC;
}
static int stub_close(ftdi_handle_t* h) {
    free(h);
    return FTDI_OK;
}
static int stub_set_params(ftdi_handle_t* h, const ftdi_params_t* p) {
    (void)h; (void)p;
    return FTDI_OK;
}
static int stub_write(ftdi_handle_t* h, const uint8_t* data, size_t n, size_t* nwritten) {
    (void)h;
    if (nwritten) *nwritten = n;
    return FTDI_OK;
}
static int stub_read(ftdi_handle_t* h, uint8_t* data, size_t cap, size_t* nread, uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!h || !data || cap == 0 || !nread) return FTDI_E_GENERIC;
    // generate a simple test frame every call
    unsigned len = (unsigned)(16 + (h->counter % 32));
    if (len > cap) len = (unsigned)cap;
    for (unsigned i=0;i<len;i++) data[i] = (uint8_t)(i + h->counter);
    *nread = len;
    h->counter += 1;
    return FTDI_OK;
}

static const ftdi_vtbl_t VSTUB = {
    .open = stub_open,
    .close = stub_close,
    .set_params = stub_set_params,
    .write = stub_write,
    .read = stub_read
};

const ftdi_vtbl_t* ftdi_get_driver(ftdi_driver_kind_t kind) {
    switch (kind) {
        case FTDI_DRIVER_STUB: return &VSTUB;
        default: return &VSTUB;
    }
}
