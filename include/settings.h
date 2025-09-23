#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Settings Categories
typedef enum {
    SETTINGS_CATEGORY_GENERAL = 0,
    SETTINGS_CATEGORY_CONNECTION = 1,
    SETTINGS_CATEGORY_OBD = 2,
    SETTINGS_CATEGORY_DASHBOARD = 3,
    SETTINGS_CATEGORY_LOGGING = 4,
    SETTINGS_CATEGORY_UI = 5,
    SETTINGS_CATEGORY_VEHICLE = 6,
    SETTINGS_CATEGORY_ADVANCED = 7
} settings_category_t;

// Vehicle Types
typedef enum {
    VEHICLE_TYPE_UNKNOWN = 0,
    VEHICLE_TYPE_CAR = 1,
    VEHICLE_TYPE_TRUCK = 2,
    VEHICLE_TYPE_MOTORCYCLE = 3,
    VEHICLE_TYPE_BUS = 4,
    VEHICLE_TYPE_OTHER = 5
} vehicle_type_t;

// OBD Protocols
typedef enum {
    OBD_PROTOCOL_AUTO = 0,
    OBD_PROTOCOL_ISO_9141_2_SETTING = 1,
    OBD_PROTOCOL_ISO_14230_4 = 2,
    OBD_PROTOCOL_ISO_15765_4 = 3,
    OBD_PROTOCOL_J1850_PWM = 4,
    OBD_PROTOCOL_J1850_VPW = 5
} obd_protocol_setting_t;

// Connection Settings
typedef struct {
    char device_path[256];
    uint32_t baudrate;
    uint32_t timeout_ms;
    bool auto_connect;
    bool auto_reconnect;
    uint32_t reconnect_interval_ms;
    bool enable_esp32;
    bool enable_ftdi;
    char esp32_ip[16];
    uint16_t esp32_port;
    char esp32_bluetooth_address[18];
} connection_settings_t;

// OBD Settings
typedef struct {
    obd_protocol_setting_t protocol;
    uint32_t response_timeout_ms;
    uint32_t request_interval_ms;
    bool auto_detect_protocol;
    bool enable_live_data;
    bool enable_dtc_scanning;
    bool enable_freeze_frame;
    uint8_t supported_pids[32];
    size_t supported_pid_count;
} obd_settings_t;

// Dashboard Settings
typedef struct {
    bool show_gauges;
    bool show_charts;
    bool show_alerts;
    uint32_t refresh_interval_ms;
    bool auto_refresh;
    float warning_threshold_percent;
    float critical_threshold_percent;
    uint32_t chart_history_size;
    bool enable_sound_alerts;
    bool enable_visual_alerts;
} dashboard_settings_t;

// Logging Settings
typedef struct {
    bool enable_logging;
    char log_directory[256];
    char log_filename[128];
    uint32_t log_interval_ms;
    uint32_t max_file_size_mb;
    uint32_t max_files;
    bool compress_old_files;
    bool include_timestamps;
    bool include_metadata;
    bool auto_rotate_logs;
} logging_settings_t;

// UI Settings
typedef struct {
    char theme[32];
    char language[8];
    bool dark_mode;
    uint32_t window_width;
    uint32_t window_height;
    bool window_maximized;
    bool show_status_bar;
    bool show_toolbar;
    bool enable_tooltips;
    uint32_t font_size;
    char font_family[64];
} ui_settings_t;

// Vehicle Settings
typedef struct {
    vehicle_type_t type;
    char make[32];
    char model[64];
    char year[8];
    char vin[17];
    char engine_type[32];
    uint32_t engine_displacement;
    char fuel_type[16];
    uint32_t transmission_type;
    bool is_hybrid;
    bool is_electric;
} vehicle_settings_t;

// Advanced Settings
typedef struct {
    bool enable_debug_mode;
    bool enable_verbose_logging;
    uint32_t debug_level;
    bool enable_performance_monitoring;
    bool enable_memory_monitoring;
    uint32_t max_memory_usage_mb;
    bool enable_crash_recovery;
    bool enable_data_validation;
    char custom_config_file[256];
} advanced_settings_t;

// Main Settings Structure
typedef struct {
    connection_settings_t connection;
    obd_settings_t obd;
    dashboard_settings_t dashboard;
    logging_settings_t logging;
    ui_settings_t ui;
    vehicle_settings_t vehicle;
    advanced_settings_t advanced;
    
    // Global settings
    char config_file[256];
    bool auto_save;
    uint32_t auto_save_interval_ms;
    bool enable_backup;
    uint32_t backup_interval_hours;
    char backup_directory[256];
    
    // Metadata
    uint64_t last_modified;
    uint64_t last_saved;
    uint32_t version;
    bool is_initialized;
} settings_t;

// Settings Manager
typedef struct {
    settings_t settings;
    bool is_loaded;
    bool is_dirty;
    char config_file_path[256];
    uint64_t last_auto_save;
} settings_manager_t;

// Function prototypes
settings_manager_t* settings_manager_create(void);
void settings_manager_destroy(settings_manager_t* manager);

// Initialization
int settings_manager_init(settings_manager_t* manager, const char* config_file);
int settings_manager_load_defaults(settings_manager_t* manager);
int settings_manager_load_from_file(settings_manager_t* manager, const char* filename);
int settings_manager_save_to_file(settings_manager_t* manager, const char* filename);

// Settings access
const settings_t* settings_manager_get_settings(settings_manager_t* manager);
int settings_manager_set_settings(settings_manager_t* manager, const settings_t* settings);

// Category-specific access
const connection_settings_t* settings_manager_get_connection_settings(settings_manager_t* manager);
int settings_manager_set_connection_settings(settings_manager_t* manager, const connection_settings_t* settings);
const obd_settings_t* settings_manager_get_obd_settings(settings_manager_t* manager);
int settings_manager_set_obd_settings(settings_manager_t* manager, const obd_settings_t* settings);
const dashboard_settings_t* settings_manager_get_dashboard_settings(settings_manager_t* manager);
int settings_manager_set_dashboard_settings(settings_manager_t* manager, const dashboard_settings_t* settings);
const logging_settings_t* settings_manager_get_logging_settings(settings_manager_t* manager);
int settings_manager_set_logging_settings(settings_manager_t* manager, const logging_settings_t* settings);
const ui_settings_t* settings_manager_get_ui_settings(settings_manager_t* manager);
int settings_manager_set_ui_settings(settings_manager_t* manager, const ui_settings_t* settings);
const vehicle_settings_t* settings_manager_get_vehicle_settings(settings_manager_t* manager);
int settings_manager_set_vehicle_settings(settings_manager_t* manager, const vehicle_settings_t* settings);
const advanced_settings_t* settings_manager_get_advanced_settings(settings_manager_t* manager);
int settings_manager_set_advanced_settings(settings_manager_t* manager, const advanced_settings_t* settings);

// Individual setting access
int settings_manager_get_string_setting(settings_manager_t* manager, settings_category_t category, const char* key, char* value, size_t value_size);
int settings_manager_set_string_setting(settings_manager_t* manager, settings_category_t category, const char* key, const char* value);
int settings_manager_get_int_setting(settings_manager_t* manager, settings_category_t category, const char* key, int32_t* value);
int settings_manager_set_int_setting(settings_manager_t* manager, settings_category_t category, const char* key, int32_t value);
int settings_manager_get_bool_setting(settings_manager_t* manager, settings_category_t category, const char* key, bool* value);
int settings_manager_set_bool_setting(settings_manager_t* manager, settings_category_t category, const char* key, bool value);
int settings_manager_get_float_setting(settings_manager_t* manager, settings_category_t category, const char* key, float* value);
int settings_manager_set_float_setting(settings_manager_t* manager, settings_category_t category, const char* key, float value);

// File operations
int settings_manager_export_settings(settings_manager_t* manager, const char* filename);
int settings_manager_import_settings(settings_manager_t* manager, const char* filename);
int settings_manager_backup_settings(settings_manager_t* manager);
int settings_manager_restore_settings(settings_manager_t* manager, const char* backup_file);
int settings_manager_reset_to_defaults(settings_manager_t* manager);

// Validation
bool settings_manager_validate_settings(settings_manager_t* manager);
bool settings_manager_validate_connection_settings(const connection_settings_t* settings);
bool settings_manager_validate_obd_settings(const obd_settings_t* settings);
bool settings_manager_validate_dashboard_settings(const dashboard_settings_t* settings);
bool settings_manager_validate_logging_settings(const logging_settings_t* settings);
bool settings_manager_validate_ui_settings(const ui_settings_t* settings);
bool settings_manager_validate_vehicle_settings(const vehicle_settings_t* settings);
bool settings_manager_validate_advanced_settings(const advanced_settings_t* settings);

// Utility functions
const char* settings_category_to_string(settings_category_t category);
const char* vehicle_type_to_string(vehicle_type_t type);
const char* obd_protocol_setting_to_string(obd_protocol_setting_t protocol);
int settings_manager_get_setting_count(settings_manager_t* manager, settings_category_t category);
int settings_manager_get_all_settings(settings_manager_t* manager, settings_category_t category, char** keys, char** values, size_t max_count, size_t* count);

// Pre-defined configurations
int settings_manager_set_default_config(settings_manager_t* manager);
int settings_manager_set_high_performance_config(settings_manager_t* manager);
int settings_manager_set_debug_config(settings_manager_t* manager);
int settings_manager_set_monitoring_config(settings_manager_t* manager);
int settings_manager_set_development_config(settings_manager_t* manager);

// Auto-save functionality
int settings_manager_enable_auto_save(settings_manager_t* manager, bool enable, uint32_t interval_ms);
int settings_manager_auto_save(settings_manager_t* manager);
bool settings_manager_is_dirty(settings_manager_t* manager);
int settings_manager_mark_dirty(settings_manager_t* manager);
int settings_manager_clear_dirty(settings_manager_t* manager);

// Version management
uint32_t settings_manager_get_version(settings_manager_t* manager);
int settings_manager_set_version(settings_manager_t* manager, uint32_t version);
int settings_manager_migrate_settings(settings_manager_t* manager, uint32_t from_version, uint32_t to_version);

// Error handling
const char* settings_manager_get_last_error(settings_manager_t* manager);
int settings_manager_set_error(settings_manager_t* manager, const char* error_message);
int settings_manager_clear_error(settings_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H
