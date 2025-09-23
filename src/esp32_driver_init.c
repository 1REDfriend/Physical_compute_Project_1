#include "esp32_driver.h"

// Initialize ESP32 driver
int esp32_driver_init(esp32_driver_t* driver)
{
    if (!driver)
        return -1;
    
    // Initialize driver state
    driver->status = ESP32_STATUS_DISCONNECTED;
    driver->is_connected = false;
    driver->connection_handle = NULL;
    driver->bytes_received = 0;
    driver->bytes_sent = 0;
    driver->last_activity = 0;
    driver->last_error[0] = '\0';
    
    // Initialize config
    driver->config.connection_type = ESP32_CONNECTION_SERIAL;
    driver->config.device_path[0] = '\0';
    driver->config.wifi_ssid[0] = '\0';
    driver->config.wifi_password[0] = '\0';
    driver->config.wifi_ip[0] = '\0';
    driver->config.wifi_port = 80;
    driver->config.bluetooth_address[0] = '\0';
    driver->config.baudrate = 115200;
    driver->config.timeout_ms = 5000;
    
    return 0;
}
