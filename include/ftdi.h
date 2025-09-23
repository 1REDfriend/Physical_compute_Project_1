#ifndef FTDI_H
#define FTDI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // FTDI Error Codes
    typedef enum
    {
        FTDI_OK = 0,
        FTDI_E_INVALID_PARAM = -1,
        FTDI_E_MEMORY = -2,
        FTDI_E_TIMEOUT = -3,
        FTDI_E_DEVICE_NOT_FOUND = -4,
        FTDI_E_DEVICE_NOT_OPENED = -5,
        FTDI_E_IO_ERROR = -6,
        FTDI_E_INSUFFICIENT_RESOURCES = -7,
        FTDI_E_INVALID_BAUDRATE = -8,
        FTDI_E_DEVICE_NOT_LISTED = -9,
        FTDI_E_DEVICE_NOT_OPENED_FOR_ERASE = -10,
        FTDI_E_DEVICE_NOT_OPENED_FOR_WRITE = -11,
        FTDI_E_FAILED_TO_WRITE_DEVICE = -12,
        FTDI_E_EEPROM_READ_FAILED = -13,
        FTDI_E_EEPROM_WRITE_FAILED = -14,
        FTDI_E_EEPROM_ERASE_FAILED = -15,
        FTDI_E_EEPROM_NOT_PRESENT = -16,
        FTDI_E_EEPROM_NOT_PROGRAMMED = -17,
        FTDI_E_INVALID_ARGS = -18,
        FTDI_E_NOT_SUPPORTED = -19,
        FTDI_E_OTHER_ERROR = -20
    } ftdi_status_t;

    // FTDI Driver Types
    typedef enum
    {
        FTDI_DRIVER_STUB = 0,
        FTDI_DRIVER_D2XX = 1
    } ftdi_driver_kind_t;

    // FTDI Device Info
    typedef struct
    {
        uint16_t vid;
        uint16_t pid;
        char manufacturer[64];
        char product[64];
        char serial[64];
    } ftdi_device_info_t;

    // FTDI Parameters
    typedef struct
    {
        uint32_t baudrate;
        uint8_t data_bits;
        uint8_t stop_bits;
        uint8_t parity;
        uint32_t latency_ms;
    } ftdi_params_t;

    // FTDI Handle
    typedef struct
    {
        bool is_open;
        uint32_t baudrate;
        uint8_t data_bits;
        uint8_t stop_bits;
        uint8_t parity;
        uint32_t latency_ms;
        void *driver_data;
    } ftdi_handle_t;

    // FTDI Driver Virtual Table
    typedef struct
    {
        int (*open)(ftdi_handle_t **handle);
        int (*close)(ftdi_handle_t *handle);
        int (*set_params)(ftdi_handle_t *handle, const ftdi_params_t *params);
        int (*read)(ftdi_handle_t *handle, uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms);
        int (*write)(ftdi_handle_t *handle, const uint8_t *data, size_t data_size, size_t *bytes_written);
    } ftdi_vtbl_t;

    // Function prototypes
    const ftdi_vtbl_t *ftdi_get_driver(ftdi_driver_kind_t kind);
    int ftdi_enumerate_devices(ftdi_device_info_t *devices, size_t max_devices, size_t *device_count);

    // Utility functions
    const char *ftdi_status_to_string(ftdi_status_t status);

#ifdef __cplusplus
}
#endif

#endif // FTDI_H
