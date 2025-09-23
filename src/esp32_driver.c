#include "esp32_driver.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#ifdef _MSC_VER
#pragma comment(lib, "setupapi.lib")
#endif
#elif defined(__linux__)
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#elif defined(__APPLE__)
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOKitLib.h>
#endif

// Platform-specific connection handle
#ifdef _WIN32
typedef struct
{
    HANDLE handle;
    DCB dcb;
    COMMTIMEOUTS timeouts;
} esp32_serial_handle_t;
#else
typedef struct
{
    int fd;
    struct termios old_termios;
    struct termios new_termios;
} esp32_serial_handle_t;
#endif

// ESP32 Driver Implementation
esp32_driver_t *esp32_driver_create(void)
{
    esp32_driver_t *driver = (esp32_driver_t *)calloc(1, sizeof(esp32_driver_t));
    if (!driver)
        return NULL;

    // Initialize default configuration
    driver->config.connection_type = ESP32_CONNECTION_SERIAL;
    driver->config.baudrate = 115200;
    driver->config.timeout_ms = 5000;
    driver->status = ESP32_STATUS_DISCONNECTED;
    driver->is_connected = false;

    return driver;
}

void esp32_driver_destroy(esp32_driver_t *driver)
{
    if (!driver)
        return;

    if (driver->is_connected)
    {
        esp32_driver_disconnect(driver);
    }

    if (driver->connection_handle)
    {
        free(driver->connection_handle);
    }

    free(driver);
}

// Configuration functions
int esp32_driver_set_config(esp32_driver_t *driver, const esp32_config_t *config)
{
    if (!driver || !config)
        return -1;

    memcpy(&driver->config, config, sizeof(esp32_config_t));
    return 0;
}

int esp32_driver_set_serial_config(esp32_driver_t *driver, const char *device_path, uint32_t baudrate)
{
    if (!driver || !device_path)
        return -1;

    driver->config.connection_type = ESP32_CONNECTION_SERIAL;
    strncpy(driver->config.device_path, device_path, sizeof(driver->config.device_path) - 1);
    driver->config.baudrate = baudrate;

    return 0;
}

int esp32_driver_set_wifi_config(esp32_driver_t *driver, const char *ssid, const char *password, const char *ip, uint16_t port)
{
    if (!driver || !ssid || !password || !ip)
        return -1;

    driver->config.connection_type = ESP32_CONNECTION_WIFI;
    strncpy(driver->config.wifi_ssid, ssid, sizeof(driver->config.wifi_ssid) - 1);
    strncpy(driver->config.wifi_password, password, sizeof(driver->config.wifi_password) - 1);
    strncpy(driver->config.wifi_ip, ip, sizeof(driver->config.wifi_ip) - 1);
    driver->config.wifi_port = port;

    return 0;
}

int esp32_driver_set_bluetooth_config(esp32_driver_t *driver, const char *address)
{
    if (!driver || !address)
        return -1;

    driver->config.connection_type = ESP32_CONNECTION_BLUETOOTH;
    strncpy(driver->config.bluetooth_address, address, sizeof(driver->config.bluetooth_address) - 1);

    return 0;
}

// Connection management
int esp32_driver_connect(esp32_driver_t *driver)
{
    if (!driver)
        return -1;

    if (driver->is_connected)
    {
        return 0; // Already connected
    }

    driver->status = ESP32_STATUS_CONNECTING;

    int result = -1;
    switch (driver->config.connection_type)
    {
    case ESP32_CONNECTION_SERIAL:
#ifdef _WIN32
        result = esp32_serial_connect_win32(driver);
#elif defined(__linux__)
        result = esp32_serial_connect_linux(driver);
#elif defined(__APPLE__)
        result = esp32_serial_connect_macos(driver);
#endif
        break;

    case ESP32_CONNECTION_WIFI:
        // TODO: Implement WiFi connection
        snprintf(driver->last_error, sizeof(driver->last_error), "WiFi connection not implemented yet");
        result = -1;
        break;

    case ESP32_CONNECTION_BLUETOOTH:
        // TODO: Implement Bluetooth connection
        snprintf(driver->last_error, sizeof(driver->last_error), "Bluetooth connection not implemented yet");
        result = -1;
        break;
    }

    if (result == 0)
    {
        driver->status = ESP32_STATUS_CONNECTED;
        driver->is_connected = true;
        driver->last_activity = time(NULL);
    }
    else
    {
        driver->status = ESP32_STATUS_ERROR;
        driver->is_connected = false;
    }

    return result;
}

int esp32_driver_disconnect(esp32_driver_t *driver)
{
    if (!driver)
        return -1;

    if (!driver->is_connected)
    {
        return 0; // Already disconnected
    }

    int result = -1;
    switch (driver->config.connection_type)
    {
    case ESP32_CONNECTION_SERIAL:
#ifdef _WIN32
        result = esp32_serial_disconnect_win32(driver);
#elif defined(__linux__)
        result = esp32_serial_disconnect_linux(driver);
#elif defined(__APPLE__)
        result = esp32_serial_disconnect_macos(driver);
#endif
        break;

    case ESP32_CONNECTION_WIFI:
    case ESP32_CONNECTION_BLUETOOTH:
        // TODO: Implement disconnection
        result = 0;
        break;
    }

    driver->status = ESP32_STATUS_DISCONNECTED;
    driver->is_connected = false;

    return result;
}

bool esp32_driver_is_connected(esp32_driver_t *driver)
{
    return driver ? driver->is_connected : false;
}

esp32_status_t esp32_driver_get_status(esp32_driver_t *driver)
{
    return driver ? driver->status : ESP32_STATUS_ERROR;
}

// Data transmission
int esp32_driver_send(esp32_driver_t *driver, const uint8_t *data, size_t length)
{
    if (!driver || !data || length == 0)
        return -1;

    if (!driver->is_connected)
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Not connected");
        return -1;
    }

    int result = -1;
    switch (driver->config.connection_type)
    {
    case ESP32_CONNECTION_SERIAL:
#ifdef _WIN32
        result = esp32_serial_send_win32(driver, data, length);
#elif defined(__linux__)
        result = esp32_serial_send_linux(driver, data, length);
#elif defined(__APPLE__)
        result = esp32_serial_send_macos(driver, data, length);
#endif
        break;

    case ESP32_CONNECTION_WIFI:
    case ESP32_CONNECTION_BLUETOOTH:
        // TODO: Implement network transmission
        result = -1;
        break;
    }

    if (result == 0)
    {
        driver->bytes_sent += length;
        driver->last_activity = time(NULL);
    }

    return result;
}

int esp32_driver_receive(esp32_driver_t *driver, uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!driver || !buffer || buffer_size == 0 || !received_length)
        return -1;

    if (!driver->is_connected)
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Not connected");
        return -1;
    }

    *received_length = 0;

    int result = -1;
    switch (driver->config.connection_type)
    {
    case ESP32_CONNECTION_SERIAL:
#ifdef _WIN32
        result = esp32_serial_receive_win32(driver, buffer, buffer_size, received_length);
#elif defined(__linux__)
        result = esp32_serial_receive_linux(driver, buffer, buffer_size, received_length);
#elif defined(__APPLE__)
        result = esp32_serial_receive_macos(driver, buffer, buffer_size, received_length);
#endif
        break;

    case ESP32_CONNECTION_WIFI:
    case ESP32_CONNECTION_BLUETOOTH:
        // TODO: Implement network reception
        result = -1;
        break;
    }

    if (result == 0 && *received_length > 0)
    {
        driver->bytes_received += *received_length;
        driver->last_activity = time(NULL);
    }

    return result;
}

int esp32_driver_receive_packet(esp32_driver_t *driver, esp32_packet_t *packet)
{
    if (!driver || !packet)
        return -1;

    uint8_t buffer[1024];
    size_t received_length;

    int result = esp32_driver_receive(driver, buffer, sizeof(buffer), &received_length);
    if (result != 0 || received_length == 0)
    {
        return -1;
    }

    // Simple packet parsing (can be enhanced)
    packet->data = (uint8_t *)malloc(received_length);
    if (!packet->data)
        return -1;

    memcpy(packet->data, buffer, received_length);
    packet->length = received_length;
    packet->timestamp = time(NULL);
    packet->is_obd_data = (received_length >= 3); // Basic OBD data detection
    packet->source_id = 0;                        // Default source

    return 0;
}

// OBD-II specific functions
int esp32_driver_send_obd_command(esp32_driver_t *driver, uint8_t mode, uint8_t pid)
{
    if (!driver)
        return -1;

    uint8_t command[4];
    command[0] = 0x33; // Header
    command[1] = mode;
    command[2] = pid;
    command[3] = command[0] ^ command[1] ^ command[2]; // Checksum

    return esp32_driver_send(driver, command, sizeof(command));
}

int esp32_driver_request_live_data(esp32_driver_t *driver, uint8_t pid)
{
    return esp32_driver_send_obd_command(driver, 0x01, pid);
}

int esp32_driver_request_dtc_codes(esp32_driver_t *driver)
{
    return esp32_driver_send_obd_command(driver, 0x03, 0x00);
}

int esp32_driver_clear_dtc_codes(esp32_driver_t *driver)
{
    return esp32_driver_send_obd_command(driver, 0x04, 0x00);
}

// Utility functions
const char *esp32_connection_type_to_string(esp32_connection_type_t type)
{
    switch (type)
    {
    case ESP32_CONNECTION_SERIAL:
        return "Serial";
    case ESP32_CONNECTION_WIFI:
        return "WiFi";
    case ESP32_CONNECTION_BLUETOOTH:
        return "Bluetooth";
    default:
        return "Unknown";
    }
}

const char *esp32_status_to_string(esp32_status_t status)
{
    switch (status)
    {
    case ESP32_STATUS_DISCONNECTED:
        return "Disconnected";
    case ESP32_STATUS_CONNECTING:
        return "Connecting";
    case ESP32_STATUS_CONNECTED:
        return "Connected";
    case ESP32_STATUS_ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

const char *esp32_driver_get_last_error(esp32_driver_t *driver)
{
    return driver ? driver->last_error : "Invalid driver";
}

uint64_t esp32_driver_get_bytes_received(esp32_driver_t *driver)
{
    return driver ? driver->bytes_received : 0;
}

uint64_t esp32_driver_get_bytes_sent(esp32_driver_t *driver)
{
    return driver ? driver->bytes_sent : 0;
}

// Platform-specific implementations
#ifdef _WIN32
int esp32_serial_connect_win32(esp32_driver_t *driver)
{
    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)calloc(1, sizeof(esp32_serial_handle_t));
    if (!handle)
        return -1;

    handle->handle = CreateFileA(driver->config.device_path,
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);

    if (handle->handle == INVALID_HANDLE_VALUE)
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to open serial port: %s", driver->config.device_path);
        free(handle);
        return -1;
    }

    // Configure serial port
    handle->dcb.DCBlength = sizeof(DCB);
    GetCommState(handle->handle, &handle->dcb);
    handle->dcb.BaudRate = driver->config.baudrate;
    handle->dcb.ByteSize = 8;
    handle->dcb.Parity = NOPARITY;
    handle->dcb.StopBits = ONESTOPBIT;
    handle->dcb.fDtrControl = DTR_CONTROL_DISABLE;
    handle->dcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(handle->handle, &handle->dcb))
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to configure serial port");
        CloseHandle(handle->handle);
        free(handle);
        return -1;
    }

    // Set timeouts
    handle->timeouts.ReadIntervalTimeout = 50;
    handle->timeouts.ReadTotalTimeoutConstant = 50;
    handle->timeouts.ReadTotalTimeoutMultiplier = 10;
    handle->timeouts.WriteTotalTimeoutConstant = 50;
    handle->timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(handle->handle, &handle->timeouts))
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to set serial timeouts");
        CloseHandle(handle->handle);
        free(handle);
        return -1;
    }

    driver->connection_handle = handle;
    return 0;
}

int esp32_serial_disconnect_win32(esp32_driver_t *driver)
{
    if (!driver->connection_handle)
        return 0;

    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)driver->connection_handle;
    CloseHandle(handle->handle);
    free(handle);
    driver->connection_handle = NULL;

    return 0;
}

int esp32_serial_send_win32(esp32_driver_t *driver, const uint8_t *data, size_t length)
{
    if (!driver->connection_handle)
        return -1;

    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)driver->connection_handle;
    DWORD bytes_written;

    if (!WriteFile(handle->handle, data, (DWORD)length, &bytes_written, NULL))
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to write to serial port");
        return -1;
    }

    return (bytes_written == length) ? 0 : -1;
}

int esp32_serial_receive_win32(esp32_driver_t *driver, uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!driver->connection_handle)
        return -1;

    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)driver->connection_handle;
    DWORD bytes_read;

    if (!ReadFile(handle->handle, buffer, (DWORD)buffer_size, &bytes_read, NULL))
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to read from serial port");
        return -1;
    }

    *received_length = bytes_read;
    return 0;
}
#endif

#ifdef __linux__
int esp32_serial_connect_linux(esp32_driver_t *driver)
{
    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)calloc(1, sizeof(esp32_serial_handle_t));
    if (!handle)
        return -1;

    handle->fd = open(driver->config.device_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (handle->fd == -1)
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to open serial port: %s", driver->config.device_path);
        free(handle);
        return -1;
    }

    // Configure serial port
    tcgetattr(handle->fd, &handle->old_termios);
    handle->new_termios = handle->old_termios;

    cfsetospeed(&handle->new_termios, B115200);
    cfsetispeed(&handle->new_termios, B115200);

    handle->new_termios.c_cflag = CS8 | CLOCAL | CREAD;
    handle->new_termios.c_iflag = IGNPAR;
    handle->new_termios.c_oflag = 0;
    handle->new_termios.c_lflag = 0;

    tcflush(handle->fd, TCIFLUSH);
    tcsetattr(handle->fd, TCSANOW, &handle->new_termios);

    driver->connection_handle = handle;
    return 0;
}

int esp32_serial_disconnect_linux(esp32_driver_t *driver)
{
    if (!driver->connection_handle)
        return 0;

    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)driver->connection_handle;
    tcsetattr(handle->fd, TCSANOW, &handle->old_termios);
    close(handle->fd);
    free(handle);
    driver->connection_handle = NULL;

    return 0;
}

int esp32_serial_send_linux(esp32_driver_t *driver, const uint8_t *data, size_t length)
{
    if (!driver->connection_handle)
        return -1;

    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)driver->connection_handle;
    ssize_t bytes_written = write(handle->fd, data, length);

    if (bytes_written == -1)
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to write to serial port");
        return -1;
    }

    return (bytes_written == (ssize_t)length) ? 0 : -1;
}

int esp32_serial_receive_linux(esp32_driver_t *driver, uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!driver->connection_handle)
        return -1;

    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)driver->connection_handle;
    ssize_t bytes_read = read(handle->fd, buffer, buffer_size);

    if (bytes_read == -1)
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to read from serial port");
        return -1;
    }

    *received_length = (size_t)bytes_read;
    return 0;
}
#endif

#ifdef __APPLE__
int esp32_serial_connect_macos(esp32_driver_t *driver)
{
    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)calloc(1, sizeof(esp32_serial_handle_t));
    if (!handle)
        return -1;

    handle->fd = open(driver->config.device_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (handle->fd == -1)
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to open serial port: %s", driver->config.device_path);
        free(handle);
        return -1;
    }

    // Configure serial port (similar to Linux)
    tcgetattr(handle->fd, &handle->old_termios);
    handle->new_termios = handle->old_termios;

    cfsetospeed(&handle->new_termios, B115200);
    cfsetispeed(&handle->new_termios, B115200);

    handle->new_termios.c_cflag = CS8 | CLOCAL | CREAD;
    handle->new_termios.c_iflag = IGNPAR;
    handle->new_termios.c_oflag = 0;
    handle->new_termios.c_lflag = 0;

    tcflush(handle->fd, TCIFLUSH);
    tcsetattr(handle->fd, TCSANOW, &handle->new_termios);

    driver->connection_handle = handle;
    return 0;
}

int esp32_serial_disconnect_macos(esp32_driver_t *driver)
{
    if (!driver->connection_handle)
        return 0;

    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)driver->connection_handle;
    tcsetattr(handle->fd, TCSANOW, &handle->old_termios);
    close(handle->fd);
    free(handle);
    driver->connection_handle = NULL;

    return 0;
}

int esp32_serial_send_macos(esp32_driver_t *driver, const uint8_t *data, size_t length)
{
    if (!driver->connection_handle)
        return -1;

    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)driver->connection_handle;
    ssize_t bytes_written = write(handle->fd, data, length);

    if (bytes_written == -1)
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to write to serial port");
        return -1;
    }

    return (bytes_written == (ssize_t)length) ? 0 : -1;
}

int esp32_serial_receive_macos(esp32_driver_t *driver, uint8_t *buffer, size_t buffer_size, size_t *received_length)
{
    if (!driver->connection_handle)
        return -1;

    esp32_serial_handle_t *handle = (esp32_serial_handle_t *)driver->connection_handle;
    ssize_t bytes_read = read(handle->fd, buffer, buffer_size);

    if (bytes_read == -1)
    {
        snprintf(driver->last_error, sizeof(driver->last_error), "Failed to read from serial port");
        return -1;
    }

    *received_length = (size_t)bytes_read;
    return 0;
}
#endif
