#include "ftdi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// FTDI stub driver implementation
static int ftdi_stub_open(ftdi_handle_t **handle)
{
    if (!handle)
        return FTDI_E_INVALID_PARAM;

    *handle = (ftdi_handle_t *)calloc(1, sizeof(ftdi_handle_t));
    if (!*handle)
        return FTDI_E_MEMORY;

    (*handle)->is_open = true;
    return FTDI_OK;
}

static int ftdi_stub_close(ftdi_handle_t *handle)
{
    if (!handle)
        return FTDI_E_INVALID_PARAM;

    handle->is_open = false;
    free(handle);
    return FTDI_OK;
}

static int ftdi_stub_set_params(ftdi_handle_t *handle, const ftdi_params_t *params)
{
    if (!handle || !params)
        return FTDI_E_INVALID_PARAM;

    handle->baudrate = params->baudrate;
    handle->data_bits = params->data_bits;
    handle->stop_bits = params->stop_bits;
    handle->parity = params->parity;
    handle->latency_ms = params->latency_ms;

    return FTDI_OK;
}

static int ftdi_stub_read(ftdi_handle_t *handle, uint8_t *buffer, size_t buffer_size, size_t *bytes_read, uint32_t timeout_ms)
{
    (void)buffer_size; // Suppress unused parameter warning
    if (!handle || !buffer || !bytes_read)
        return FTDI_E_INVALID_PARAM;

    // Stub implementation - simulate reading data
    *bytes_read = 0;

    // Simulate timeout
    if (timeout_ms > 0)
    {
        // In real implementation, would wait for data or timeout
        return FTDI_E_TIMEOUT;
    }

    return FTDI_OK;
}

static int ftdi_stub_write(ftdi_handle_t *handle, const uint8_t *data, size_t data_size, size_t *bytes_written)
{
    if (!handle || !data || !bytes_written)
        return FTDI_E_INVALID_PARAM;

    // Stub implementation - simulate writing data
    *bytes_written = data_size;

    // Print to console for debugging
    printf("FTDI Stub: Writing %zu bytes: ", data_size);
    for (size_t i = 0; i < data_size && i < 16; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\n");

    return FTDI_OK;
}

// FTDI stub driver virtual table
static const ftdi_vtbl_t ftdi_stub_vtbl = {
    .open = ftdi_stub_open,
    .close = ftdi_stub_close,
    .set_params = ftdi_stub_set_params,
    .read = ftdi_stub_read,
    .write = ftdi_stub_write};

const ftdi_vtbl_t *ftdi_get_driver(ftdi_driver_kind_t kind)
{
    switch (kind)
    {
    case FTDI_DRIVER_STUB:
        return &ftdi_stub_vtbl;
    case FTDI_DRIVER_D2XX:
        // D2XX driver would be implemented separately
        return NULL;
    default:
        return NULL;
    }
}
