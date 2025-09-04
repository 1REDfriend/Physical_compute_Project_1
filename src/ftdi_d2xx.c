#include "ftdi.h"

#ifdef _WIN32
#include <windows.h>
#include "ftd2xx.h"
#include <stdlib.h>
#include <string.h>

struct ftdi_handle {
    FT_HANDLE h;
};

static int d2xx_open(ftdi_handle_t** out) {
    if (!out) return FTDI_E_GENERIC;
    *out = NULL;
    FT_STATUS st;
    DWORD num = 0;
    st = FT_CreateDeviceInfoList(&num);
    if (st != FT_OK || num == 0) return FTDI_E_NO_DEVICE;
    ftdi_handle_t* dev = (ftdi_handle_t*)calloc(1, sizeof(*dev));
    if (!dev) return FTDI_E_GENERIC;
    st = FT_Open(0, &dev->h);
    if (st != FT_OK) { free(dev); return FTDI_E_NO_DEVICE; }
    *out = dev;
    return FTDI_OK;
}

static int d2xx_close(ftdi_handle_t* d) {
    if (!d) return FTDI_OK;
    if (d->h) FT_Close(d->h);
    free(d);
    return FTDI_OK;
}

static int d2xx_set_params(ftdi_handle_t* d, const ftdi_params_t* p) {
    if (!d || !p) return FTDI_E_GENERIC;
    FT_STATUS st;
    st = FT_SetBaudRate(d->h, p->baudrate); if (st != FT_OK) return FTDI_E_IO;
    UCHAR dbits = (p->data_bits == 7) ? FT_BITS_7 : FT_BITS_8;
    UCHAR sbit  = (p->stop_bits == 2) ? FT_STOP_BITS_2 : FT_STOP_BITS_1;
    UCHAR par   = FT_PARITY_NONE;
    if (p->parity == 1) par = FT_PARITY_ODD;
    else if (p->parity == 2) par = FT_PARITY_EVEN;
    st = FT_SetDataCharacteristics(d->h, dbits, sbit, par); if (st != FT_OK) return FTDI_E_IO;
    st = FT_SetFlowControl(d->h, FT_FLOW_NONE, 0, 0); if (st != FT_OK) return FTDI_E_IO;
    st = FT_SetLatencyTimer(d->h, p->latency_ms); if (st != FT_OK) return FTDI_E_IO;
    st = FT_Purge(d->h, FT_PURGE_RX | FT_PURGE_TX); if (st != FT_OK) return FTDI_E_IO;
    st = FT_SetTimeouts(d->h, 50, 50); if (st != FT_OK) return FTDI_E_IO;
    return FTDI_OK;
}

static int d2xx_write(ftdi_handle_t* d, const uint8_t* data, size_t n, size_t* nwritten) {
    if (!d || !data || n==0) return FTDI_E_GENERIC;
    DWORD w=0;
    FT_STATUS st = FT_Write(d->h, (PVOID)data, (DWORD)n, &w);
    if (nwritten) *nwritten = (size_t)w;
    return (st == FT_OK) ? FTDI_OK : FTDI_E_IO;
}

static int d2xx_read(ftdi_handle_t* d, uint8_t* data, size_t cap, size_t* nread, uint32_t timeout_ms) {
    if (!d || !data || cap==0 || !nread) return FTDI_E_GENERIC;
    // Use blocking read with timeout configured by FT_SetTimeouts
    DWORD r=0;
    FT_STATUS st = FT_Read(d->h, data, (DWORD)cap, &r);
    if (st == FT_OK) {
        *nread = (size_t)r;
        return (r > 0) ? FTDI_OK : FTDI_E_TIMEOUT;
    } else {
        return FTDI_E_IO;
    }
}

static const ftdi_vtbl_t VD2XX = {
    .open = d2xx_open,
    .close = d2xx_close,
    .set_params = d2xx_set_params,
    .write = d2xx_write,
    .read = d2xx_read
};

#else
// Non-Windows placeholder
static const ftdi_vtbl_t VD2XX = {0};
#endif

const ftdi_vtbl_t* ftdi_get_driver(ftdi_driver_kind_t kind) {
#ifdef _WIN32
    switch (kind) {
        case FTDI_DRIVER_D2XX: return &VD2XX;
        case FTDI_DRIVER_STUB: default: break;
    }
#endif
    extern const ftdi_vtbl_t* ftdi_get_driver(ftdi_driver_kind_t kind); // resolve to stub if needed
    return ftdi_get_driver(FTDI_DRIVER_STUB);
}
