#ifndef ESP32_DRIVER_H
#define ESP32_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ESP32 Connection Types
typedef enum {
    ESP32_CONNECTION_SERIAL = 0,
    ESP32_CONNECTION_WIFI = 1,
    ESP32_CONNECTION_BLUETOOTH = 2
} esp32_connection_type_t;

// ESP32 Driver Status
typedef enum {
    ESP32_STATUS_DISCONNECTED = 0,
    ESP32_STATUS_CONNECTING = 1,
    ESP32_STATUS_CONNECTED = 2,
    ESP32_STATUS_ERROR = 3
} esp32_status_t;

// ESP32 Configuration
typedef struct {
    esp32_connection_type_t connection_type;
    char device_path[256];        // For serial connection
    char wifi_ssid[64];          // For WiFi connection
    char wifi_password[64];      // For WiFi connection
    char wifi_ip[16];            // ESP32 IP address
    uint16_t wifi_port;          // ESP32 port
    char bluetooth_address[18];  // For Bluetooth connection
    uint32_t baudrate;           // Serial baudrate
    uint32_t timeout_ms;         // Connection timeout
} esp32_config_t;

// ESP32 Driver Handle
typedef struct {
    esp32_config_t config;
    esp32_status_t status;
    void* connection_handle;     // Platform-specific connection handle
    bool is_connected;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    uint64_t last_activity;
    char last_error[256];
} esp32_driver_t;

// ESP32 Data Packet
typedef struct {
    uint8_t* data;
    size_t length;
    uint64_t timestamp;
    bool is_obd_data;
    uint8_t source_id;
} esp32_packet_t;

// Function prototypes
esp32_driver_t* esp32_driver_create(void);
void esp32_driver_destroy(esp32_driver_t* driver);
int esp32_driver_init(esp32_driver_t* driver);

// Configuration
int esp32_driver_set_config(esp32_driver_t* driver, const esp32_config_t* config);
int esp32_driver_set_serial_config(esp32_driver_t* driver, const char* device_path, uint32_t baudrate);
int esp32_driver_set_wifi_config(esp32_driver_t* driver, const char* ssid, const char* password, const char* ip, uint16_t port);
int esp32_driver_set_bluetooth_config(esp32_driver_t* driver, const char* address);

// Connection management
int esp32_driver_connect(esp32_driver_t* driver);
int esp32_driver_disconnect(esp32_driver_t* driver);
bool esp32_driver_is_connected(esp32_driver_t* driver);
esp32_status_t esp32_driver_get_status(esp32_driver_t* driver);

// Data transmission
int esp32_driver_send(esp32_driver_t* driver, const uint8_t* data, size_t length);
int esp32_driver_receive(esp32_driver_t* driver, uint8_t* buffer, size_t buffer_size, size_t* received_length);
int esp32_driver_receive_packet(esp32_driver_t* driver, esp32_packet_t* packet);

// OBD-II specific functions
int esp32_driver_send_obd_command(esp32_driver_t* driver, uint8_t mode, uint8_t pid);
int esp32_driver_request_live_data(esp32_driver_t* driver, uint8_t pid);
int esp32_driver_request_dtc_codes(esp32_driver_t* driver);
int esp32_driver_clear_dtc_codes(esp32_driver_t* driver);

// Utility functions
const char* esp32_connection_type_to_string(esp32_connection_type_t type);
const char* esp32_status_to_string(esp32_status_t status);
const char* esp32_driver_get_last_error(esp32_driver_t* driver);
uint64_t esp32_driver_get_bytes_received(esp32_driver_t* driver);
uint64_t esp32_driver_get_bytes_sent(esp32_driver_t* driver);

// Platform-specific implementations
#ifdef _WIN32
int esp32_serial_connect_win32(esp32_driver_t* driver);
int esp32_serial_disconnect_win32(esp32_driver_t* driver);
int esp32_serial_send_win32(esp32_driver_t* driver, const uint8_t* data, size_t length);
int esp32_serial_receive_win32(esp32_driver_t* driver, uint8_t* buffer, size_t buffer_size, size_t* received_length);
#endif

#ifdef __linux__
int esp32_serial_connect_linux(esp32_driver_t* driver);
int esp32_serial_disconnect_linux(esp32_driver_t* driver);
int esp32_serial_send_linux(esp32_driver_t* driver, const uint8_t* data, size_t length);
int esp32_serial_receive_linux(esp32_driver_t* driver, uint8_t* buffer, size_t buffer_size, size_t* received_length);
#endif

#ifdef __APPLE__
int esp32_serial_connect_macos(esp32_driver_t* driver);
int esp32_serial_disconnect_macos(esp32_driver_t* driver);
int esp32_serial_send_macos(esp32_driver_t* driver, const uint8_t* data, size_t length);
int esp32_serial_receive_macos(esp32_driver_t* driver, uint8_t* buffer, size_t buffer_size, size_t* received_length);
#endif

#ifdef __cplusplus
}
#endif

#endif // ESP32_DRIVER_H
