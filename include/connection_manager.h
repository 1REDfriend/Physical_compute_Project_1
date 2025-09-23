#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp32_driver.h"
#include "obd_parser.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Connection Types
    typedef enum
    {
        CONNECTION_TYPE_FTDI = 0,
        CONNECTION_TYPE_ESP32_SERIAL = 1,
        CONNECTION_TYPE_ESP32_WIFI = 2,
        CONNECTION_TYPE_ESP32_BLUETOOTH = 3,
        CONNECTION_TYPE_UNKNOWN = 4
    } connection_type_t;

    // Connection Status
    typedef enum
    {
        CONNECTION_STATUS_DISCONNECTED = 0,
        CONNECTION_STATUS_CONNECTING = 1,
        CONNECTION_STATUS_CONNECTED = 2,
        CONNECTION_STATUS_ERROR = 3,
        CONNECTION_STATUS_TIMEOUT = 4
    } connection_status_t;

    // Connection Priority
    typedef enum
    {
        CONNECTION_PRIORITY_LOW = 0,
        CONNECTION_PRIORITY_NORMAL = 1,
        CONNECTION_PRIORITY_HIGH = 2,
        CONNECTION_PRIORITY_CRITICAL = 3
    } connection_priority_t;

    // Connection Configuration
    typedef struct
    {
        connection_type_t type;
        char name[64];
        char device_path[256];
        char connection_string[256];
        uint32_t baudrate;
        uint32_t timeout_ms;
        connection_priority_t priority;
        bool auto_connect;
        bool auto_reconnect;
        uint32_t reconnect_interval_ms;
        bool is_enabled;
    } connection_config_t;

    // Connection Handle
    typedef struct
    {
        connection_config_t config;
        connection_status_t status;
        void *driver_handle; // Platform-specific driver handle
        bool is_active;
        uint64_t connect_time;
        uint64_t last_activity;
        uint64_t bytes_received;
        uint64_t bytes_sent;
        uint32_t error_count;
        char last_error[256];
        uint32_t reconnect_attempts;
        uint64_t next_reconnect_time;
    } connection_handle_t;

    // Connection Manager State
    typedef struct
    {
        connection_handle_t *connections;
        size_t connection_count;
        size_t max_connections;
        connection_handle_t *active_connection;
        connection_handle_t *primary_connection;
        bool auto_detection_enabled;
        uint32_t detection_interval_ms;
        uint64_t last_detection_time;
        bool is_initialized;
    } connection_manager_t;

    // Data Source
    typedef struct
    {
        connection_handle_t *connection;
        uint8_t *data;
        size_t data_length;
        uint64_t timestamp;
        bool is_obd_data;
        uint8_t source_id;
    } data_source_t;

    // Function prototypes
    connection_manager_t *connection_manager_create(void);
    void connection_manager_destroy(connection_manager_t *manager);

    // Initialization
    int connection_manager_init(connection_manager_t *manager, size_t max_connections);
    int connection_manager_set_auto_detection(connection_manager_t *manager, bool enabled, uint32_t interval_ms);

    // Connection management
    int connection_manager_add_connection(connection_manager_t *manager, const connection_config_t *config);
    int connection_manager_remove_connection(connection_manager_t *manager, const char *name);
    int connection_manager_connect(connection_manager_t *manager, const char *name);
    int connection_manager_disconnect(connection_manager_t *manager, const char *name);
    int connection_manager_disconnect_all(connection_manager_t *manager);

    // Connection queries
    connection_handle_t *connection_manager_get_connection(connection_manager_t *manager, const char *name);
    connection_handle_t *connection_manager_get_active_connection(connection_manager_t *manager);
    connection_handle_t *connection_manager_get_primary_connection(connection_manager_t *manager);
    size_t connection_manager_get_connection_count(connection_manager_t *manager);
    size_t connection_manager_get_active_connection_count(connection_manager_t *manager);

    // Connection operations
    int connection_manager_set_primary_connection(connection_manager_t *manager, const char *name);
    int connection_manager_switch_connection(connection_manager_t *manager, const char *name);
    int connection_manager_auto_switch_connection(connection_manager_t *manager);

    // Data operations
    int connection_manager_send_data(connection_manager_t *manager, const char *connection_name, const uint8_t *data, size_t length);
    int connection_manager_receive_data(connection_manager_t *manager, const char *connection_name, uint8_t *buffer, size_t buffer_size, size_t *received_length);
    int connection_manager_receive_from_active(connection_manager_t *manager, uint8_t *buffer, size_t buffer_size, size_t *received_length);

    // OBD-II specific operations
    int connection_manager_send_obd_command(connection_manager_t *manager, const char *connection_name, uint8_t mode, uint8_t pid);
    int connection_manager_request_live_data(connection_manager_t *manager, const char *connection_name, uint8_t pid);
    int connection_manager_request_dtc_codes(connection_manager_t *manager, const char *connection_name);
    int connection_manager_clear_dtc_codes(connection_manager_t *manager, const char *connection_name);

    // Auto-detection
    int connection_manager_start_auto_detection(connection_manager_t *manager);
    int connection_manager_stop_auto_detection(connection_manager_t *manager);
    int connection_manager_detect_connections(connection_manager_t *manager);
    int connection_manager_auto_connect_best_connection(connection_manager_t *manager);

    // Status and monitoring
    connection_status_t connection_manager_get_connection_status(connection_manager_t *manager, const char *name);
    bool connection_manager_is_connection_active(connection_manager_t *manager, const char *name);
    uint64_t connection_manager_get_connection_uptime(connection_manager_t *manager, const char *name);
    uint64_t connection_manager_get_connection_bytes_received(connection_manager_t *manager, const char *name);
    uint64_t connection_manager_get_connection_bytes_sent(connection_manager_t *manager, const char *name);
    uint32_t connection_manager_get_connection_error_count(connection_manager_t *manager, const char *name);

    // Utility functions
    const char *connection_type_to_string(connection_type_t type);
    const char *connection_status_to_string(connection_status_t status);
    const char *connection_priority_to_string(connection_priority_t priority);
    connection_type_t connection_manager_detect_connection_type(const char *device_path);
    int connection_manager_set_connection_config(connection_manager_t *manager, const char *name, const connection_config_t *config);

    // Pre-defined configurations
    int connection_manager_add_ftdi_connection(connection_manager_t *manager, const char *name, const char *device_path, uint32_t baudrate);
    int connection_manager_add_esp32_serial_connection(connection_manager_t *manager, const char *name, const char *device_path, uint32_t baudrate);
    int connection_manager_add_esp32_wifi_connection(connection_manager_t *manager, const char *name, const char *ip, uint16_t port);
    int connection_manager_add_esp32_bluetooth_connection(connection_manager_t *manager, const char *name, const char *address);

    // Error handling
    const char *connection_manager_get_last_error(connection_manager_t *manager, const char *name);
    int connection_manager_clear_errors(connection_manager_t *manager, const char *name);
    int connection_manager_reset_connection(connection_manager_t *manager, const char *name);

    // Statistics
    int connection_manager_get_total_bytes_received(connection_manager_t *manager);
    int connection_manager_get_total_bytes_sent(connection_manager_t *manager);
    int connection_manager_get_total_error_count(connection_manager_t *manager);
    int connection_manager_get_connection_statistics(connection_manager_t *manager, const char *name, uint64_t *bytes_rx, uint64_t *bytes_tx, uint32_t *error_count);

#ifdef __cplusplus
}
#endif

#endif // CONNECTION_MANAGER_H
