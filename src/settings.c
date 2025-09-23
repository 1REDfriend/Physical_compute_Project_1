#include "settings.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

// Settings Manager Implementation
settings_manager_t *settings_manager_create(void)
{
    settings_manager_t *manager = (settings_manager_t *)calloc(1, sizeof(settings_manager_t));
    if (!manager)
        return NULL;

    manager->is_loaded = false;
    manager->is_dirty = false;
    manager->last_auto_save = 0;

    return manager;
}

void settings_manager_destroy(settings_manager_t *manager)
{
    if (!manager)
        return;

    // Auto-save if dirty
    if (manager->is_dirty && manager->settings.auto_save)
    {
        settings_manager_save_to_file(manager, manager->config_file_path);
    }

    free(manager);
}

int settings_manager_init(settings_manager_t *manager, const char *config_file)
{
    if (!manager)
        return -1;

    // Set config file path
    if (config_file)
    {
        strncpy(manager->config_file_path, config_file, sizeof(manager->config_file_path) - 1);
        strncpy(manager->settings.config_file, config_file, sizeof(manager->settings.config_file) - 1);
    }
    else
    {
        strcpy(manager->config_file_path, "./config.json");
        strcpy(manager->settings.config_file, "./config.json");
    }

    // Load defaults first
    settings_manager_load_defaults(manager);

    // Try to load from file
    if (settings_manager_load_from_file(manager, manager->config_file_path) != 0)
    {
        // If loading fails, keep defaults
        manager->is_loaded = true;
    }

    manager->settings.is_initialized = true;
    return 0;
}

int settings_manager_load_defaults(settings_manager_t *manager)
{
    if (!manager)
        return -1;

    // Connection settings
    strcpy(manager->settings.connection.device_path, "/dev/ttyUSB0");
    manager->settings.connection.baudrate = 115200;
    manager->settings.connection.timeout_ms = 5000;
    manager->settings.connection.auto_connect = true;
    manager->settings.connection.auto_reconnect = true;
    manager->settings.connection.reconnect_interval_ms = 5000;
    manager->settings.connection.enable_esp32 = true;
    manager->settings.connection.enable_ftdi = true;
    strcpy(manager->settings.connection.esp32_ip, "192.168.1.100");
    manager->settings.connection.esp32_port = 8080;
    strcpy(manager->settings.connection.esp32_bluetooth_address, "00:00:00:00:00:00");

    // OBD settings
    manager->settings.obd.protocol = OBD_PROTOCOL_AUTO;
    manager->settings.obd.response_timeout_ms = 2000;
    manager->settings.obd.request_interval_ms = 1000;
    manager->settings.obd.auto_detect_protocol = true;
    manager->settings.obd.enable_live_data = true;
    manager->settings.obd.enable_dtc_scanning = true;
    manager->settings.obd.enable_freeze_frame = false;
    manager->settings.obd.supported_pid_count = 0;

    // Dashboard settings
    manager->settings.dashboard.show_gauges = true;
    manager->settings.dashboard.show_charts = true;
    manager->settings.dashboard.show_alerts = true;
    manager->settings.dashboard.refresh_interval_ms = 1000;
    manager->settings.dashboard.auto_refresh = true;
    manager->settings.dashboard.warning_threshold_percent = 80.0f;
    manager->settings.dashboard.critical_threshold_percent = 90.0f;
    manager->settings.dashboard.chart_history_size = 1000;
    manager->settings.dashboard.enable_sound_alerts = false;
    manager->settings.dashboard.enable_visual_alerts = true;

    // Logging settings
    manager->settings.logging.enable_logging = true;
    strcpy(manager->settings.logging.log_directory, "./logs");
    strcpy(manager->settings.logging.log_filename, "obd_log");
    manager->settings.logging.log_interval_ms = 1000;
    manager->settings.logging.max_file_size_mb = 10;
    manager->settings.logging.max_files = 10;
    manager->settings.logging.compress_old_files = false;
    manager->settings.logging.include_timestamps = true;
    manager->settings.logging.include_metadata = true;
    manager->settings.logging.auto_rotate_logs = true;

    // UI settings
    strcpy(manager->settings.ui.theme, "default");
    strcpy(manager->settings.ui.language, "en");
    manager->settings.ui.dark_mode = false;
    manager->settings.ui.window_width = 800;
    manager->settings.ui.window_height = 600;
    manager->settings.ui.window_maximized = false;
    manager->settings.ui.show_status_bar = true;
    manager->settings.ui.show_toolbar = true;
    manager->settings.ui.enable_tooltips = true;
    manager->settings.ui.font_size = 12;
    strcpy(manager->settings.ui.font_family, "Arial");

    // Vehicle settings
    manager->settings.vehicle.type = VEHICLE_TYPE_CAR;
    strcpy(manager->settings.vehicle.make, "Unknown");
    strcpy(manager->settings.vehicle.model, "Unknown");
    strcpy(manager->settings.vehicle.year, "2023");
    strcpy(manager->settings.vehicle.vin, "");
    strcpy(manager->settings.vehicle.engine_type, "Gasoline");
    manager->settings.vehicle.engine_displacement = 2000;
    strcpy(manager->settings.vehicle.fuel_type, "Gasoline");
    manager->settings.vehicle.transmission_type = 0;
    manager->settings.vehicle.is_hybrid = false;
    manager->settings.vehicle.is_electric = false;

    // Advanced settings
    manager->settings.advanced.enable_debug_mode = false;
    manager->settings.advanced.enable_verbose_logging = false;
    manager->settings.advanced.debug_level = 1;
    manager->settings.advanced.enable_performance_monitoring = false;
    manager->settings.advanced.enable_memory_monitoring = false;
    manager->settings.advanced.max_memory_usage_mb = 512;
    manager->settings.advanced.enable_crash_recovery = true;
    manager->settings.advanced.enable_data_validation = true;
    strcpy(manager->settings.advanced.custom_config_file, "");

    // Global settings
    manager->settings.auto_save = true;
    manager->settings.auto_save_interval_ms = 30000; // 30 seconds
    manager->settings.enable_backup = true;
    manager->settings.backup_interval_hours = 24;
    strcpy(manager->settings.backup_directory, "./backups");

    // Metadata
    manager->settings.last_modified = time(NULL);
    manager->settings.last_saved = 0;
    manager->settings.version = 1;

    return 0;
}

int settings_manager_load_from_file(settings_manager_t *manager, const char *filename)
{
    if (!manager || !filename)
        return -1;

    FILE *file = fopen(filename, "r");
    if (!file)
        return -1;

    // Simple JSON-like parsing (simplified implementation)
    char line[1024];
    while (fgets(line, sizeof(line), file))
    {
        // Remove newline
        line[strcspn(line, "\n\r")] = 0;

        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#')
            continue;

        // Parse key-value pairs (simplified)
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");

        if (key && value)
        {
            // Remove whitespace
            while (*key == ' ' || *key == '\t')
                key++;
            while (*value == ' ' || *key == '\t')
                value++;

            // Set setting based on key
            if (strcmp(key, "baudrate") == 0)
            {
                manager->settings.connection.baudrate = atoi(value);
            }
            else if (strcmp(key, "device_path") == 0)
            {
                strncpy(manager->settings.connection.device_path, value, sizeof(manager->settings.connection.device_path) - 1);
            }
            else if (strcmp(key, "auto_connect") == 0)
            {
                manager->settings.connection.auto_connect = (strcmp(value, "true") == 0);
            }
            // Add more key-value pairs as needed
        }
    }

    fclose(file);
    manager->is_loaded = true;
    return 0;
}

int settings_manager_save_to_file(settings_manager_t *manager, const char *filename)
{
    if (!manager || !filename)
        return -1;

    FILE *file = fopen(filename, "w");
    if (!file)
        return -1;

    // Write settings in simple format
    fprintf(file, "# OBD-II Reader Configuration File\n");
    time_t last_modified_time = (time_t)manager->settings.last_modified;
    fprintf(file, "# Generated on: %s\n", ctime(&last_modified_time));
    fprintf(file, "\n");

    // Connection settings
    fprintf(file, "[Connection]\n");
    fprintf(file, "device_path=%s\n", manager->settings.connection.device_path);
    fprintf(file, "baudrate=%d\n", manager->settings.connection.baudrate);
    fprintf(file, "timeout_ms=%d\n", manager->settings.connection.timeout_ms);
    fprintf(file, "auto_connect=%s\n", manager->settings.connection.auto_connect ? "true" : "false");
    fprintf(file, "auto_reconnect=%s\n", manager->settings.connection.auto_reconnect ? "true" : "false");
    fprintf(file, "reconnect_interval_ms=%d\n", manager->settings.connection.reconnect_interval_ms);
    fprintf(file, "enable_esp32=%s\n", manager->settings.connection.enable_esp32 ? "true" : "false");
    fprintf(file, "enable_ftdi=%s\n", manager->settings.connection.enable_ftdi ? "true" : "false");
    fprintf(file, "esp32_ip=%s\n", manager->settings.connection.esp32_ip);
    fprintf(file, "esp32_port=%d\n", manager->settings.connection.esp32_port);
    fprintf(file, "esp32_bluetooth_address=%s\n", manager->settings.connection.esp32_bluetooth_address);
    fprintf(file, "\n");

    // OBD settings
    fprintf(file, "[OBD]\n");
    fprintf(file, "protocol=%d\n", manager->settings.obd.protocol);
    fprintf(file, "response_timeout_ms=%d\n", manager->settings.obd.response_timeout_ms);
    fprintf(file, "request_interval_ms=%d\n", manager->settings.obd.request_interval_ms);
    fprintf(file, "auto_detect_protocol=%s\n", manager->settings.obd.auto_detect_protocol ? "true" : "false");
    fprintf(file, "enable_live_data=%s\n", manager->settings.obd.enable_live_data ? "true" : "false");
    fprintf(file, "enable_dtc_scanning=%s\n", manager->settings.obd.enable_dtc_scanning ? "true" : "false");
    fprintf(file, "enable_freeze_frame=%s\n", manager->settings.obd.enable_freeze_frame ? "true" : "false");
    fprintf(file, "\n");

    // Dashboard settings
    fprintf(file, "[Dashboard]\n");
    fprintf(file, "show_gauges=%s\n", manager->settings.dashboard.show_gauges ? "true" : "false");
    fprintf(file, "show_charts=%s\n", manager->settings.dashboard.show_charts ? "true" : "false");
    fprintf(file, "show_alerts=%s\n", manager->settings.dashboard.show_alerts ? "true" : "false");
    fprintf(file, "refresh_interval_ms=%d\n", manager->settings.dashboard.refresh_interval_ms);
    fprintf(file, "auto_refresh=%s\n", manager->settings.dashboard.auto_refresh ? "true" : "false");
    fprintf(file, "warning_threshold_percent=%.1f\n", manager->settings.dashboard.warning_threshold_percent);
    fprintf(file, "critical_threshold_percent=%.1f\n", manager->settings.dashboard.critical_threshold_percent);
    fprintf(file, "chart_history_size=%d\n", manager->settings.dashboard.chart_history_size);
    fprintf(file, "enable_sound_alerts=%s\n", manager->settings.dashboard.enable_sound_alerts ? "true" : "false");
    fprintf(file, "enable_visual_alerts=%s\n", manager->settings.dashboard.enable_visual_alerts ? "true" : "false");
    fprintf(file, "\n");

    // Logging settings
    fprintf(file, "[Logging]\n");
    fprintf(file, "enable_logging=%s\n", manager->settings.logging.enable_logging ? "true" : "false");
    fprintf(file, "log_directory=%s\n", manager->settings.logging.log_directory);
    fprintf(file, "log_filename=%s\n", manager->settings.logging.log_filename);
    fprintf(file, "log_interval_ms=%d\n", manager->settings.logging.log_interval_ms);
    fprintf(file, "max_file_size_mb=%d\n", manager->settings.logging.max_file_size_mb);
    fprintf(file, "max_files=%d\n", manager->settings.logging.max_files);
    fprintf(file, "compress_old_files=%s\n", manager->settings.logging.compress_old_files ? "true" : "false");
    fprintf(file, "include_timestamps=%s\n", manager->settings.logging.include_timestamps ? "true" : "false");
    fprintf(file, "include_metadata=%s\n", manager->settings.logging.include_metadata ? "true" : "false");
    fprintf(file, "auto_rotate_logs=%s\n", manager->settings.logging.auto_rotate_logs ? "true" : "false");
    fprintf(file, "\n");

    // UI settings
    fprintf(file, "[UI]\n");
    fprintf(file, "theme=%s\n", manager->settings.ui.theme);
    fprintf(file, "language=%s\n", manager->settings.ui.language);
    fprintf(file, "dark_mode=%s\n", manager->settings.ui.dark_mode ? "true" : "false");
    fprintf(file, "window_width=%d\n", manager->settings.ui.window_width);
    fprintf(file, "window_height=%d\n", manager->settings.ui.window_height);
    fprintf(file, "window_maximized=%s\n", manager->settings.ui.window_maximized ? "true" : "false");
    fprintf(file, "show_status_bar=%s\n", manager->settings.ui.show_status_bar ? "true" : "false");
    fprintf(file, "show_toolbar=%s\n", manager->settings.ui.show_toolbar ? "true" : "false");
    fprintf(file, "enable_tooltips=%s\n", manager->settings.ui.enable_tooltips ? "true" : "false");
    fprintf(file, "font_size=%d\n", manager->settings.ui.font_size);
    fprintf(file, "font_family=%s\n", manager->settings.ui.font_family);
    fprintf(file, "\n");

    // Vehicle settings
    fprintf(file, "[Vehicle]\n");
    fprintf(file, "type=%d\n", manager->settings.vehicle.type);
    fprintf(file, "make=%s\n", manager->settings.vehicle.make);
    fprintf(file, "model=%s\n", manager->settings.vehicle.model);
    fprintf(file, "year=%s\n", manager->settings.vehicle.year);
    fprintf(file, "vin=%s\n", manager->settings.vehicle.vin);
    fprintf(file, "engine_type=%s\n", manager->settings.vehicle.engine_type);
    fprintf(file, "engine_displacement=%d\n", manager->settings.vehicle.engine_displacement);
    fprintf(file, "fuel_type=%s\n", manager->settings.vehicle.fuel_type);
    fprintf(file, "transmission_type=%d\n", manager->settings.vehicle.transmission_type);
    fprintf(file, "is_hybrid=%s\n", manager->settings.vehicle.is_hybrid ? "true" : "false");
    fprintf(file, "is_electric=%s\n", manager->settings.vehicle.is_electric ? "true" : "false");
    fprintf(file, "\n");

    // Advanced settings
    fprintf(file, "[Advanced]\n");
    fprintf(file, "enable_debug_mode=%s\n", manager->settings.advanced.enable_debug_mode ? "true" : "false");
    fprintf(file, "enable_verbose_logging=%s\n", manager->settings.advanced.enable_verbose_logging ? "true" : "false");
    fprintf(file, "debug_level=%d\n", manager->settings.advanced.debug_level);
    fprintf(file, "enable_performance_monitoring=%s\n", manager->settings.advanced.enable_performance_monitoring ? "true" : "false");
    fprintf(file, "enable_memory_monitoring=%s\n", manager->settings.advanced.enable_memory_monitoring ? "true" : "false");
    fprintf(file, "max_memory_usage_mb=%d\n", manager->settings.advanced.max_memory_usage_mb);
    fprintf(file, "enable_crash_recovery=%s\n", manager->settings.advanced.enable_crash_recovery ? "true" : "false");
    fprintf(file, "enable_data_validation=%s\n", manager->settings.advanced.enable_data_validation ? "true" : "false");
    fprintf(file, "custom_config_file=%s\n", manager->settings.advanced.custom_config_file);
    fprintf(file, "\n");

    // Global settings
    fprintf(file, "[Global]\n");
    fprintf(file, "auto_save=%s\n", manager->settings.auto_save ? "true" : "false");
    fprintf(file, "auto_save_interval_ms=%d\n", manager->settings.auto_save_interval_ms);
    fprintf(file, "enable_backup=%s\n", manager->settings.enable_backup ? "true" : "false");
    fprintf(file, "backup_interval_hours=%d\n", manager->settings.backup_interval_hours);
    fprintf(file, "backup_directory=%s\n", manager->settings.backup_directory);
    fprintf(file, "version=%d\n", manager->settings.version);

    fclose(file);

    manager->settings.last_saved = time(NULL);
    manager->is_dirty = false;

    return 0;
}

const settings_t *settings_manager_get_settings(settings_manager_t *manager)
{
    return manager ? &manager->settings : NULL;
}

int settings_manager_set_settings(settings_manager_t *manager, const settings_t *settings)
{
    if (!manager || !settings)
        return -1;

    manager->settings = *settings;
    manager->settings.last_modified = time(NULL);
    manager->is_dirty = true;

    return 0;
}

const connection_settings_t *settings_manager_get_connection_settings(settings_manager_t *manager)
{
    return manager ? &manager->settings.connection : NULL;
}

int settings_manager_set_connection_settings(settings_manager_t *manager, const connection_settings_t *settings)
{
    if (!manager || !settings)
        return -1;

    manager->settings.connection = *settings;
    manager->settings.last_modified = time(NULL);
    manager->is_dirty = true;

    return 0;
}

const obd_settings_t *settings_manager_get_obd_settings(settings_manager_t *manager)
{
    return manager ? &manager->settings.obd : NULL;
}

int settings_manager_set_obd_settings(settings_manager_t *manager, const obd_settings_t *settings)
{
    if (!manager || !settings)
        return -1;

    manager->settings.obd = *settings;
    manager->settings.last_modified = time(NULL);
    manager->is_dirty = true;

    return 0;
}

const dashboard_settings_t *settings_manager_get_dashboard_settings(settings_manager_t *manager)
{
    return manager ? &manager->settings.dashboard : NULL;
}

int settings_manager_set_dashboard_settings(settings_manager_t *manager, const dashboard_settings_t *settings)
{
    if (!manager || !settings)
        return -1;

    manager->settings.dashboard = *settings;
    manager->settings.last_modified = time(NULL);
    manager->is_dirty = true;

    return 0;
}

const logging_settings_t *settings_manager_get_logging_settings(settings_manager_t *manager)
{
    return manager ? &manager->settings.logging : NULL;
}

int settings_manager_set_logging_settings(settings_manager_t *manager, const logging_settings_t *settings)
{
    if (!manager || !settings)
        return -1;

    manager->settings.logging = *settings;
    manager->settings.last_modified = time(NULL);
    manager->is_dirty = true;

    return 0;
}

const ui_settings_t *settings_manager_get_ui_settings(settings_manager_t *manager)
{
    return manager ? &manager->settings.ui : NULL;
}

int settings_manager_set_ui_settings(settings_manager_t *manager, const ui_settings_t *settings)
{
    if (!manager || !settings)
        return -1;

    manager->settings.ui = *settings;
    manager->settings.last_modified = time(NULL);
    manager->is_dirty = true;

    return 0;
}

const vehicle_settings_t *settings_manager_get_vehicle_settings(settings_manager_t *manager)
{
    return manager ? &manager->settings.vehicle : NULL;
}

int settings_manager_set_vehicle_settings(settings_manager_t *manager, const vehicle_settings_t *settings)
{
    if (!manager || !settings)
        return -1;

    manager->settings.vehicle = *settings;
    manager->settings.last_modified = time(NULL);
    manager->is_dirty = true;

    return 0;
}

const advanced_settings_t *settings_manager_get_advanced_settings(settings_manager_t *manager)
{
    return manager ? &manager->settings.advanced : NULL;
}

int settings_manager_set_advanced_settings(settings_manager_t *manager, const advanced_settings_t *settings)
{
    if (!manager || !settings)
        return -1;

    manager->settings.advanced = *settings;
    manager->settings.last_modified = time(NULL);
    manager->is_dirty = true;

    return 0;
}

// Utility functions
const char *settings_category_to_string(settings_category_t category)
{
    switch (category)
    {
    case SETTINGS_CATEGORY_GENERAL:
        return "General";
    case SETTINGS_CATEGORY_CONNECTION:
        return "Connection";
    case SETTINGS_CATEGORY_OBD:
        return "OBD";
    case SETTINGS_CATEGORY_DASHBOARD:
        return "Dashboard";
    case SETTINGS_CATEGORY_LOGGING:
        return "Logging";
    case SETTINGS_CATEGORY_UI:
        return "UI";
    case SETTINGS_CATEGORY_VEHICLE:
        return "Vehicle";
    case SETTINGS_CATEGORY_ADVANCED:
        return "Advanced";
    default:
        return "Unknown";
    }
}

const char *vehicle_type_to_string(vehicle_type_t type)
{
    switch (type)
    {
    case VEHICLE_TYPE_UNKNOWN:
        return "Unknown";
    case VEHICLE_TYPE_CAR:
        return "Car";
    case VEHICLE_TYPE_TRUCK:
        return "Truck";
    case VEHICLE_TYPE_MOTORCYCLE:
        return "Motorcycle";
    case VEHICLE_TYPE_BUS:
        return "Bus";
    case VEHICLE_TYPE_OTHER:
        return "Other";
    default:
        return "Unknown";
    }
}

const char *obd_protocol_setting_to_string(obd_protocol_setting_t protocol)
{
    switch (protocol)
    {
    case OBD_PROTOCOL_AUTO:
        return "Auto";
    case OBD_PROTOCOL_ISO_9141_2_SETTING:
        return "ISO 9141-2";
    case OBD_PROTOCOL_ISO_14230_4:
        return "ISO 14230-4";
    case OBD_PROTOCOL_ISO_15765_4:
        return "ISO 15765-4";
    case OBD_PROTOCOL_J1850_PWM:
        return "J1850 PWM";
    case OBD_PROTOCOL_J1850_VPW:
        return "J1850 VPW";
    default:
        return "Unknown";
    }
}

// Pre-defined configurations
int settings_manager_set_default_config(settings_manager_t *manager)
{
    if (!manager)
        return -1;

    settings_manager_load_defaults(manager);
    return 0;
}

int settings_manager_set_high_performance_config(settings_manager_t *manager)
{
    if (!manager)
        return -1;

    settings_manager_load_defaults(manager);

    // High performance settings
    manager->settings.connection.baudrate = 460800;
    manager->settings.connection.timeout_ms = 1000;
    manager->settings.obd.response_timeout_ms = 500;
    manager->settings.obd.request_interval_ms = 100;
    manager->settings.dashboard.refresh_interval_ms = 50;
    manager->settings.logging.log_interval_ms = 100;
    manager->settings.advanced.enable_performance_monitoring = true;
    manager->settings.advanced.enable_memory_monitoring = true;

    return 0;
}

int settings_manager_set_debug_config(settings_manager_t *manager)
{
    if (!manager)
        return -1;

    settings_manager_load_defaults(manager);

    // Debug settings
    manager->settings.advanced.enable_debug_mode = true;
    manager->settings.advanced.enable_verbose_logging = true;
    manager->settings.advanced.debug_level = 3;
    manager->settings.logging.enable_logging = true;
    manager->settings.logging.include_metadata = true;
    manager->settings.logging.auto_rotate_logs = true;

    return 0;
}

int settings_manager_set_monitoring_config(settings_manager_t *manager)
{
    if (!manager)
        return -1;

    settings_manager_load_defaults(manager);

    // Monitoring settings
    manager->settings.dashboard.show_gauges = true;
    manager->settings.dashboard.show_charts = false;
    manager->settings.dashboard.show_alerts = true;
    manager->settings.dashboard.refresh_interval_ms = 5000;
    manager->settings.logging.enable_logging = true;
    manager->settings.logging.log_interval_ms = 5000;
    manager->settings.obd.request_interval_ms = 5000;

    return 0;
}

int settings_manager_set_development_config(settings_manager_t *manager)
{
    if (!manager)
        return -1;

    settings_manager_load_defaults(manager);

    // Development settings
    manager->settings.advanced.enable_debug_mode = true;
    manager->settings.advanced.enable_verbose_logging = true;
    manager->settings.advanced.debug_level = 5;
    manager->settings.advanced.enable_performance_monitoring = true;
    manager->settings.advanced.enable_memory_monitoring = true;
    manager->settings.advanced.enable_crash_recovery = true;
    manager->settings.advanced.enable_data_validation = true;
    manager->settings.logging.enable_logging = true;
    manager->settings.logging.include_metadata = true;

    return 0;
}

// Auto-save functionality
int settings_manager_enable_auto_save(settings_manager_t *manager, bool enable, uint32_t interval_ms)
{
    if (!manager)
        return -1;

    manager->settings.auto_save = enable;
    manager->settings.auto_save_interval_ms = interval_ms;

    return 0;
}

int settings_manager_auto_save(settings_manager_t *manager)
{
    if (!manager || !manager->settings.auto_save)
        return 0;

    uint64_t current_time = time(NULL);
    if (current_time - manager->last_auto_save >= (manager->settings.auto_save_interval_ms / 1000))
    {
        if (manager->is_dirty)
        {
            settings_manager_save_to_file(manager, manager->config_file_path);
            manager->last_auto_save = current_time;
        }
    }

    return 0;
}

bool settings_manager_is_dirty(settings_manager_t *manager)
{
    return manager ? manager->is_dirty : false;
}

int settings_manager_mark_dirty(settings_manager_t *manager)
{
    if (!manager)
        return -1;

    manager->is_dirty = true;
    manager->settings.last_modified = time(NULL);

    return 0;
}

int settings_manager_clear_dirty(settings_manager_t *manager)
{
    if (!manager)
        return -1;

    manager->is_dirty = false;

    return 0;
}

// Validation functions
bool settings_manager_validate_settings(settings_manager_t *manager)
{
    if (!manager)
        return false;

    return settings_manager_validate_connection_settings(&manager->settings.connection) &&
           settings_manager_validate_obd_settings(&manager->settings.obd) &&
           settings_manager_validate_dashboard_settings(&manager->settings.dashboard) &&
           settings_manager_validate_logging_settings(&manager->settings.logging) &&
           settings_manager_validate_ui_settings(&manager->settings.ui) &&
           settings_manager_validate_vehicle_settings(&manager->settings.vehicle) &&
           settings_manager_validate_advanced_settings(&manager->settings.advanced);
}

bool settings_manager_validate_connection_settings(const connection_settings_t *settings)
{
    if (!settings)
        return false;

    if (settings->baudrate == 0 || settings->baudrate > 2000000)
        return false;
    if (settings->timeout_ms == 0 || settings->timeout_ms > 60000)
        return false;
    if (settings->reconnect_interval_ms == 0 || settings->reconnect_interval_ms > 300000)
        return false;
    if (settings->esp32_port == 0 || settings->esp32_port > 65535)
        return false;

    return true;
}

bool settings_manager_validate_obd_settings(const obd_settings_t *settings)
{
    if (!settings)
        return false;

    if (settings->response_timeout_ms == 0 || settings->response_timeout_ms > 30000)
        return false;
    if (settings->request_interval_ms == 0 || settings->request_interval_ms > 60000)
        return false;

    return true;
}

bool settings_manager_validate_dashboard_settings(const dashboard_settings_t *settings)
{
    if (!settings)
        return false;

    if (settings->refresh_interval_ms == 0 || settings->refresh_interval_ms > 60000)
        return false;
    if (settings->warning_threshold_percent < 0 || settings->warning_threshold_percent > 100)
        return false;
    if (settings->critical_threshold_percent < 0 || settings->critical_threshold_percent > 100)
        return false;
    if (settings->warning_threshold_percent >= settings->critical_threshold_percent)
        return false;
    if (settings->chart_history_size == 0 || settings->chart_history_size > 100000)
        return false;

    return true;
}

bool settings_manager_validate_logging_settings(const logging_settings_t *settings)
{
    if (!settings)
        return false;

    if (settings->log_interval_ms == 0 || settings->log_interval_ms > 60000)
        return false;
    if (settings->max_file_size_mb == 0 || settings->max_file_size_mb > 1024)
        return false;
    if (settings->max_files == 0 || settings->max_files > 1000)
        return false;

    return true;
}

bool settings_manager_validate_ui_settings(const ui_settings_t *settings)
{
    if (!settings)
        return false;

    if (settings->window_width == 0 || settings->window_width > 4096)
        return false;
    if (settings->window_height == 0 || settings->window_height > 4096)
        return false;
    if (settings->font_size == 0 || settings->font_size > 72)
        return false;

    return true;
}

bool settings_manager_validate_vehicle_settings(const vehicle_settings_t *settings)
{
    if (!settings)
        return false;

    if (settings->engine_displacement == 0 || settings->engine_displacement > 10000)
        return false;

    return true;
}

bool settings_manager_validate_advanced_settings(const advanced_settings_t *settings)
{
    if (!settings)
        return false;

    if (settings->debug_level > 5)
        return false;
    if (settings->max_memory_usage_mb == 0 || settings->max_memory_usage_mb > 8192)
        return false;

    return true;
}

// Version management
uint32_t settings_manager_get_version(settings_manager_t *manager)
{
    return manager ? manager->settings.version : 0;
}

int settings_manager_set_version(settings_manager_t *manager, uint32_t version)
{
    if (!manager)
        return -1;

    manager->settings.version = version;
    manager->settings.last_modified = time(NULL);
    manager->is_dirty = true;

    return 0;
}

int settings_manager_migrate_settings(settings_manager_t *manager, uint32_t from_version, uint32_t to_version)
{
    if (!manager)
        return -1;

    // Simple migration logic
    if (from_version < to_version)
    {
        // Add new settings or modify existing ones
        if (from_version < 2)
        {
            // Add new settings for version 2
            manager->settings.advanced.enable_crash_recovery = true;
            manager->settings.advanced.enable_data_validation = true;
        }

        manager->settings.version = to_version;
        manager->settings.last_modified = time(NULL);
        manager->is_dirty = true;
    }

    return 0;
}
