#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "obd_parser.h"
#include "obd_commands.h"

#ifdef __cplusplus
extern "C" {
#endif

// Dashboard Data Types
typedef enum {
    DASHBOARD_DATA_ENGINE_RPM = 0,
    DASHBOARD_DATA_VEHICLE_SPEED = 1,
    DASHBOARD_DATA_COOLANT_TEMP = 2,
    DASHBOARD_DATA_INTAKE_AIR_TEMP = 3,
    DASHBOARD_DATA_THROTTLE_POSITION = 4,
    DASHBOARD_DATA_FUEL_LEVEL = 5,
    DASHBOARD_DATA_ENGINE_LOAD = 6,
    DASHBOARD_DATA_AMBIENT_TEMP = 7,
    DASHBOARD_DATA_COUNT = 8
} dashboard_data_type_t;

// Dashboard Data Point
typedef struct {
    float value;
    uint64_t timestamp;
    bool is_valid;
    const char* unit;
    float min_value;
    float max_value;
    float warning_threshold;
    float critical_threshold;
} dashboard_data_point_t;

// Dashboard Gauge
typedef struct {
    dashboard_data_type_t data_type;
    const char* title;
    float current_value;
    float min_value;
    float max_value;
    float warning_threshold;
    float critical_threshold;
    bool is_warning;
    bool is_critical;
    const char* unit;
    uint64_t last_update;
} dashboard_gauge_t;

// Dashboard Chart Data
typedef struct {
    float* values;
    uint64_t* timestamps;
    size_t capacity;
    size_t count;
    size_t current_index;
    bool is_full;
} dashboard_chart_t;

// Dashboard Alert
typedef struct {
    dashboard_data_type_t data_type;
    const char* message;
    uint64_t timestamp;
    bool is_active;
    bool is_acknowledged;
} dashboard_alert_t;

// Dashboard Configuration
typedef struct {
    bool auto_refresh;
    uint32_t refresh_interval_ms;
    bool show_gauges;
    bool show_charts;
    bool show_alerts;
    uint32_t chart_history_size;
    float warning_threshold_percent;
    float critical_threshold_percent;
} dashboard_config_t;

// Dashboard State
typedef struct {
    dashboard_gauge_t gauges[DASHBOARD_DATA_COUNT];
    dashboard_chart_t charts[DASHBOARD_DATA_COUNT];
    dashboard_alert_t alerts[16];
    size_t alert_count;
    dashboard_config_t config;
    bool is_initialized;
    uint64_t last_refresh;
    bool connection_active;
} dashboard_state_t;

// Function prototypes
dashboard_state_t* dashboard_create(void);
void dashboard_destroy(dashboard_state_t* dashboard);

// Initialization
int dashboard_init(dashboard_state_t* dashboard, const dashboard_config_t* config);
int dashboard_set_config(dashboard_state_t* dashboard, const dashboard_config_t* config);

// Data management
int dashboard_update_data(dashboard_state_t* dashboard, dashboard_data_type_t type, float value, uint64_t timestamp);
int dashboard_update_from_obd(dashboard_state_t* dashboard, const obd_parsed_data_t* obd_data);
int dashboard_clear_data(dashboard_state_t* dashboard);

// Gauge operations
const dashboard_gauge_t* dashboard_get_gauge(dashboard_state_t* dashboard, dashboard_data_type_t type);
int dashboard_set_gauge_limits(dashboard_state_t* dashboard, dashboard_data_type_t type, float min_val, float max_val);
int dashboard_set_gauge_thresholds(dashboard_state_t* dashboard, dashboard_data_type_t type, float warning, float critical);

// Chart operations
const dashboard_chart_t* dashboard_get_chart(dashboard_state_t* dashboard, dashboard_data_type_t type);
int dashboard_add_chart_point(dashboard_state_t* dashboard, dashboard_data_type_t type, float value, uint64_t timestamp);
int dashboard_clear_chart(dashboard_state_t* dashboard, dashboard_data_type_t type);

// Alert management
int dashboard_add_alert(dashboard_state_t* dashboard, dashboard_data_type_t type, const char* message);
int dashboard_acknowledge_alert(dashboard_state_t* dashboard, size_t alert_index);
int dashboard_clear_alerts(dashboard_state_t* dashboard);
size_t dashboard_get_active_alert_count(dashboard_state_t* dashboard);
const dashboard_alert_t* dashboard_get_alert(dashboard_state_t* dashboard, size_t index);

// Status and monitoring
bool dashboard_is_connection_active(dashboard_state_t* dashboard);
uint64_t dashboard_get_last_update_time(dashboard_state_t* dashboard);
int dashboard_set_connection_status(dashboard_state_t* dashboard, bool active);

// Utility functions
const char* dashboard_data_type_to_string(dashboard_data_type_t type);
const char* dashboard_data_type_to_unit(dashboard_data_type_t type);
float dashboard_get_data_type_min_value(dashboard_data_type_t type);
float dashboard_get_data_type_max_value(dashboard_data_type_t type);

// Pre-defined configurations
int dashboard_set_default_config(dashboard_state_t* dashboard);
int dashboard_set_high_performance_config(dashboard_state_t* dashboard);
int dashboard_set_monitoring_config(dashboard_state_t* dashboard);

#ifdef __cplusplus
}
#endif

#endif // DASHBOARD_H
