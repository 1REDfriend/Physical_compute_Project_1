#ifndef FTDI_IFACE_H
#define FTDI_IFACE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ftdi_handle ftdi_handle_t;

typedef struct ftdi_params {
    uint32_t baudrate;     // ถ้าเป็น D2XX/UART ให้ตั้งค่าได้, ถ้าเป็น FIFO/อื่นๆ อาจไม่ใช้
    uint8_t  data_bits;    // 7/8
    uint8_t  stop_bits;    // 1/2
    uint8_t  parity;       // 0=none,1=odd,2=even
    uint8_t  latency_ms;   // D2XX latency timer (1-16), ถ้าไม่รองรับให้ ignore
} ftdi_params_t;

typedef struct ftdi_vtbl {
    int  (*open)(ftdi_handle_t** out);                       // เปิดอุปกรณ์ตัวแรก/ตาม policy ภายใน
    int  (*close)(ftdi_handle_t* h);
    int  (*set_params)(ftdi_handle_t* h, const ftdi_params_t* p);
    int  (*write)(ftdi_handle_t* h, const uint8_t* data, size_t n, size_t* nwritten);
    int  (*read)(ftdi_handle_t* h, uint8_t* data, size_t cap, size_t* nread, uint32_t timeout_ms);
} ftdi_vtbl_t;

// error codes (simple)
enum {
    FTDI_OK = 0,
    FTDI_E_GENERIC = -1,
    FTDI_E_TIMEOUT = -2,
    FTDI_E_NO_DEVICE = -3,
    FTDI_E_IO = -4,
    FTDI_E_UNSUPPORTED = -5
};

// Driver selection
typedef enum {
    FTDI_DRIVER_STUB = 0,
    FTDI_DRIVER_D2XX = 1
} ftdi_driver_kind_t;

// get vtable by kind
const ftdi_vtbl_t* ftdi_get_driver(ftdi_driver_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif // FTDI_IFACE_H
