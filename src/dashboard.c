#include "dashboard.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

// Dashboard Implementation
dashboard_state_t *dashboard_create(void)
{
    dashboard_state_t *dashboard = (dashboard_state_t *)calloc(1, sizeof(dashboard_state_t));
    if (!dashboard)
        return NULL;

    dashboard->is_initialized = false;
    dashboard->alert_count = 0;
    dashboard->connection_active = false;
    dashboard->last_refresh = 0;

    return dashboard;
}

void dashboard_destroy(dashboard_state_t *dashboard)
{
    if (!dashboard)
        return;

    // Free chart data
    for (int i = 0; i < DASHBOARD_DATA_COUNT; i++)
    {
        if (dashboard->charts[i].values)
        {
            free(dashboard->charts[i].values);
        }
        if (dashboard->charts[i].timestamps)
        {
            free(dashboard->charts[i].timestamps);
        }
    }

    free(dashboard);
}

int dashboard_init(dashboard_state_t *dashboard, const dashboard_config_t *config)
{
    if (!dashboard)
        return -1;

    // Initialize gauges
    for (int i = 0; i < DASHBOARD_DATA_COUNT; i++)
    {
        dashboard->gauges[i].data_type = (dashboard_data_type_t)i;
        dashboard->gauges[i].title = dashboard_data_type_to_string((dashboard_data_type_t)i);
        dashboard->gauges[i].unit = dashboard_data_type_to_unit((dashboard_data_type_t)i);
        dashboard->gauges[i].min_value = dashboard_get_data_type_min_value((dashboard_data_type_t)i);
        dashboard->gauges[i].max_value = dashboard_get_data_type_max_value((dashboard_data_type_t)i);
        dashboard->gauges[i].warning_threshold = dashboard->gauges[i].max_value * 0.8f;
        dashboard->gauges[i].critical_threshold = dashboard->gauges[i].max_value * 0.9f;
        dashboard->gauges[i].current_value = 0.0f;
        dashboard->gauges[i].is_warning = false;
        dashboard->gauges[i].is_critical = false;
        dashboard->gauges[i].last_update = 0;
    }

    // Initialize charts
    for (int i = 0; i < DASHBOARD_DATA_COUNT; i++)
    {
        dashboard->charts[i].capacity = config ? config->chart_history_size : 1000;
        dashboard->charts[i].count = 0;
        dashboard->charts[i].current_index = 0;
        dashboard->charts[i].is_full = false;

        dashboard->charts[i].values = (float *)malloc(dashboard->charts[i].capacity * sizeof(float));
        dashboard->charts[i].timestamps = (uint64_t *)malloc(dashboard->charts[i].capacity * sizeof(uint64_t));

        if (!dashboard->charts[i].values || !dashboard->charts[i].timestamps)
        {
            return -1;
        }
    }

    // Set configuration
    if (config)
    {
        dashboard->config = *config;
    }
    else
    {
        dashboard_set_default_config(dashboard);
    }

    dashboard->is_initialized = true;
    return 0;
}

int dashboard_set_config(dashboard_state_t *dashboard, const dashboard_config_t *config)
{
    if (!dashboard || !config)
        return -1;

    dashboard->config = *config;
    return 0;
}

int dashboard_update_data(dashboard_state_t *dashboard, dashboard_data_type_t type, float value, uint64_t timestamp)
{
    if (!dashboard || !dashboard->is_initialized)
        return -1;

    if (type >= DASHBOARD_DATA_COUNT)
        return -1;

    // Update gauge
    dashboard->gauges[type].current_value = value;
    dashboard->gauges[type].last_update = timestamp;

    // Check thresholds
    dashboard->gauges[type].is_warning = (value >= dashboard->gauges[type].warning_threshold);
    dashboard->gauges[type].is_critical = (value >= dashboard->gauges[type].critical_threshold);

    // Add to chart
    dashboard_add_chart_point(dashboard, type, value, timestamp);

    // Check for alerts
    if (dashboard->gauges[type].is_critical)
    {
        char alert_message[256];
        snprintf(alert_message, sizeof(alert_message), "%s is critical: %.1f %s",
                 dashboard->gauges[type].title, value, dashboard->gauges[type].unit);
        dashboard_add_alert(dashboard, type, alert_message);
    }
    else if (dashboard->gauges[type].is_warning)
    {
        char alert_message[256];
        snprintf(alert_message, sizeof(alert_message), "%s is high: %.1f %s",
                 dashboard->gauges[type].title, value, dashboard->gauges[type].unit);
        dashboard_add_alert(dashboard, type, alert_message);
    }

    dashboard->last_refresh = timestamp;
    return 0;
}

int dashboard_update_from_obd(dashboard_state_t *dashboard, const obd_parsed_data_t *obd_data)
{
    if (!dashboard || !obd_data)
        return -1;

    if (obd_data->mode != OBD_MODE_01_LIVE_DATA)
        return -1;

    float value = 0.0f;
    // const char *unit = "";
    dashboard_data_type_t data_type = DASHBOARD_DATA_COUNT;

    // Map OBD PID to dashboard data type
    switch (obd_data->pid)
    {
    case 0x0C: // Engine RPM
        data_type = DASHBOARD_DATA_ENGINE_RPM;
        value = ((obd_data->payload[0] * 256) + obd_data->payload[1]) / 4.0f;
        break;
    case 0x0D: // Vehicle Speed
        data_type = DASHBOARD_DATA_VEHICLE_SPEED;
        value = (float)obd_data->payload[0];
        break;
    case 0x05: // Coolant Temperature
        data_type = DASHBOARD_DATA_COOLANT_TEMP;
        value = obd_data->payload[0] - 40.0f;
        break;
    case 0x0F: // Intake Air Temperature
        data_type = DASHBOARD_DATA_INTAKE_AIR_TEMP;
        value = obd_data->payload[0] - 40.0f;
        break;
    case 0x11: // Throttle Position
        data_type = DASHBOARD_DATA_THROTTLE_POSITION;
        value = (obd_data->payload[0] * 100.0f) / 255.0f;
        break;
    case 0x2F: // Fuel Level
        data_type = DASHBOARD_DATA_FUEL_LEVEL;
        value = (obd_data->payload[0] * 100.0f) / 255.0f;
        break;
    case 0x04: // Engine Load
        data_type = DASHBOARD_DATA_ENGINE_LOAD;
        value = (obd_data->payload[0] * 100.0f) / 255.0f;
        break;
    case 0x46: // Ambient Air Temperature
        data_type = DASHBOARD_DATA_AMBIENT_TEMP;
        value = obd_data->payload[0] - 40.0f;
        break;
    default:
        return -1; // Unknown PID
    }

    if (data_type < DASHBOARD_DATA_COUNT)
    {
        return dashboard_update_data(dashboard, data_type, value, obd_data->timestamp);
    }

    return -1;
}

int dashboard_clear_data(dashboard_state_t *dashboard)
{
    if (!dashboard)
        return -1;

    // Clear all gauges
    for (int i = 0; i < DASHBOARD_DATA_COUNT; i++)
    {
        dashboard->gauges[i].current_value = 0.0f;
        dashboard->gauges[i].is_warning = false;
        dashboard->gauges[i].is_critical = false;
        dashboard->gauges[i].last_update = 0;
    }

    // Clear all charts
    for (int i = 0; i < DASHBOARD_DATA_COUNT; i++)
    {
        dashboard_clear_chart(dashboard, (dashboard_data_type_t)i);
    }

    // Clear all alerts
    dashboard_clear_alerts(dashboard);

    return 0;
}

const dashboard_gauge_t *dashboard_get_gauge(dashboard_state_t *dashboard, dashboard_data_type_t type)
{
    if (!dashboard || type >= DASHBOARD_DATA_COUNT)
        return NULL;
    return &dashboard->gauges[type];
}

int dashboard_set_gauge_limits(dashboard_state_t *dashboard, dashboard_data_type_t type, float min_val, float max_val)
{
    if (!dashboard || type >= DASHBOARD_DATA_COUNT)
        return -1;

    dashboard->gauges[type].min_value = min_val;
    dashboard->gauges[type].max_value = max_val;

    return 0;
}

int dashboard_set_gauge_thresholds(dashboard_state_t *dashboard, dashboard_data_type_t type, float warning, float critical)
{
    if (!dashboard || type >= DASHBOARD_DATA_COUNT)
        return -1;

    dashboard->gauges[type].warning_threshold = warning;
    dashboard->gauges[type].critical_threshold = critical;

    return 0;
}

const dashboard_chart_t *dashboard_get_chart(dashboard_state_t *dashboard, dashboard_data_type_t type)
{
    if (!dashboard || type >= DASHBOARD_DATA_COUNT)
        return NULL;
    return &dashboard->charts[type];
}

int dashboard_add_chart_point(dashboard_state_t *dashboard, dashboard_data_type_t type, float value, uint64_t timestamp)
{
    if (!dashboard || type >= DASHBOARD_DATA_COUNT)
        return -1;

    dashboard_chart_t *chart = &dashboard->charts[type];

    chart->values[chart->current_index] = value;
    chart->timestamps[chart->current_index] = timestamp;

    chart->current_index = (chart->current_index + 1) % chart->capacity;

    if (!chart->is_full)
    {
        chart->count++;
        if (chart->count >= chart->capacity)
        {
            chart->is_full = true;
        }
    }

    return 0;
}

int dashboard_clear_chart(dashboard_state_t *dashboard, dashboard_data_type_t type)
{
    if (!dashboard || type >= DASHBOARD_DATA_COUNT)
        return -1;

    dashboard_chart_t *chart = &dashboard->charts[type];
    chart->count = 0;
    chart->current_index = 0;
    chart->is_full = false;

    return 0;
}

int dashboard_add_alert(dashboard_state_t *dashboard, dashboard_data_type_t type, const char *message)
{
    if (!dashboard || !message)
        return -1;

    if (dashboard->alert_count >= 16)
        return -1; // Max alerts

    dashboard_alert_t *alert = &dashboard->alerts[dashboard->alert_count];
    alert->data_type = type;
    alert->message = message;
    alert->timestamp = time(NULL);
    alert->is_active = true;
    alert->is_acknowledged = false;

    dashboard->alert_count++;
    return 0;
}

int dashboard_acknowledge_alert(dashboard_state_t *dashboard, size_t alert_index)
{
    if (!dashboard || alert_index >= dashboard->alert_count)
        return -1;

    dashboard->alerts[alert_index].is_acknowledged = true;
    return 0;
}

int dashboard_clear_alerts(dashboard_state_t *dashboard)
{
    if (!dashboard)
        return -1;

    dashboard->alert_count = 0;
    return 0;
}

size_t dashboard_get_active_alert_count(dashboard_state_t *dashboard)
{
    if (!dashboard)
        return 0;

    size_t count = 0;
    for (size_t i = 0; i < dashboard->alert_count; i++)
    {
        if (dashboard->alerts[i].is_active && !dashboard->alerts[i].is_acknowledged)
        {
            count++;
        }
    }

    return count;
}

const dashboard_alert_t *dashboard_get_alert(dashboard_state_t *dashboard, size_t index)
{
    if (!dashboard || index >= dashboard->alert_count)
        return NULL;
    return &dashboard->alerts[index];
}

bool dashboard_is_connection_active(dashboard_state_t *dashboard)
{
    return dashboard ? dashboard->connection_active : false;
}

uint64_t dashboard_get_last_update_time(dashboard_state_t *dashboard)
{
    return dashboard ? dashboard->last_refresh : 0;
}

int dashboard_set_connection_status(dashboard_state_t *dashboard, bool active)
{
    if (!dashboard)
        return -1;

    dashboard->connection_active = active;
    return 0;
}

const char *dashboard_data_type_to_string(dashboard_data_type_t type)
{
    switch (type)
    {
    case DASHBOARD_DATA_ENGINE_RPM:
        return "Engine RPM";
    case DASHBOARD_DATA_VEHICLE_SPEED:
        return "Vehicle Speed";
    case DASHBOARD_DATA_COOLANT_TEMP:
        return "Coolant Temperature";
    case DASHBOARD_DATA_INTAKE_AIR_TEMP:
        return "Intake Air Temperature";
    case DASHBOARD_DATA_THROTTLE_POSITION:
        return "Throttle Position";
    case DASHBOARD_DATA_FUEL_LEVEL:
        return "Fuel Level";
    case DASHBOARD_DATA_ENGINE_LOAD:
        return "Engine Load";
    case DASHBOARD_DATA_AMBIENT_TEMP:
        return "Ambient Temperature";
    default:
        return "Unknown";
    }
}

const char *dashboard_data_type_to_unit(dashboard_data_type_t type)
{
    switch (type)
    {
    case DASHBOARD_DATA_ENGINE_RPM:
        return "RPM";
    case DASHBOARD_DATA_VEHICLE_SPEED:
        return "km/h";
    case DASHBOARD_DATA_COOLANT_TEMP:
        return "°C";
    case DASHBOARD_DATA_INTAKE_AIR_TEMP:
        return "°C";
    case DASHBOARD_DATA_THROTTLE_POSITION:
        return "%";
    case DASHBOARD_DATA_FUEL_LEVEL:
        return "%";
    case DASHBOARD_DATA_ENGINE_LOAD:
        return "%";
    case DASHBOARD_DATA_AMBIENT_TEMP:
        return "°C";
    default:
        return "";
    }
}

float dashboard_get_data_type_min_value(dashboard_data_type_t type)
{
    switch (type)
    {
    case DASHBOARD_DATA_ENGINE_RPM:
        return 0.0f;
    case DASHBOARD_DATA_VEHICLE_SPEED:
        return 0.0f;
    case DASHBOARD_DATA_COOLANT_TEMP:
        return -40.0f;
    case DASHBOARD_DATA_INTAKE_AIR_TEMP:
        return -40.0f;
    case DASHBOARD_DATA_THROTTLE_POSITION:
        return 0.0f;
    case DASHBOARD_DATA_FUEL_LEVEL:
        return 0.0f;
    case DASHBOARD_DATA_ENGINE_LOAD:
        return 0.0f;
    case DASHBOARD_DATA_AMBIENT_TEMP:
        return -40.0f;
    default:
        return 0.0f;
    }
}

float dashboard_get_data_type_max_value(dashboard_data_type_t type)
{
    switch (type)
    {
    case DASHBOARD_DATA_ENGINE_RPM:
        return 8000.0f;
    case DASHBOARD_DATA_VEHICLE_SPEED:
        return 255.0f;
    case DASHBOARD_DATA_COOLANT_TEMP:
        return 215.0f;
    case DASHBOARD_DATA_INTAKE_AIR_TEMP:
        return 215.0f;
    case DASHBOARD_DATA_THROTTLE_POSITION:
        return 100.0f;
    case DASHBOARD_DATA_FUEL_LEVEL:
        return 100.0f;
    case DASHBOARD_DATA_ENGINE_LOAD:
        return 100.0f;
    case DASHBOARD_DATA_AMBIENT_TEMP:
        return 215.0f;
    default:
        return 100.0f;
    }
}

int dashboard_set_default_config(dashboard_state_t *dashboard)
{
    if (!dashboard)
        return -1;

    dashboard->config.auto_refresh = true;
    dashboard->config.refresh_interval_ms = 1000;
    dashboard->config.show_gauges = true;
    dashboard->config.show_charts = true;
    dashboard->config.show_alerts = true;
    dashboard->config.chart_history_size = 1000;
    dashboard->config.warning_threshold_percent = 80.0f;
    dashboard->config.critical_threshold_percent = 90.0f;

    return 0;
}

int dashboard_set_high_performance_config(dashboard_state_t *dashboard)
{
    if (!dashboard)
        return -1;

    dashboard->config.auto_refresh = true;
    dashboard->config.refresh_interval_ms = 100; // Faster refresh
    dashboard->config.show_gauges = true;
    dashboard->config.show_charts = true;
    dashboard->config.show_alerts = true;
    dashboard->config.chart_history_size = 5000; // More history
    dashboard->config.warning_threshold_percent = 85.0f;
    dashboard->config.critical_threshold_percent = 95.0f;

    return 0;
}

int dashboard_set_monitoring_config(dashboard_state_t *dashboard)
{
    if (!dashboard)
        return -1;

    dashboard->config.auto_refresh = true;
    dashboard->config.refresh_interval_ms = 5000; // Slower refresh
    dashboard->config.show_gauges = true;
    dashboard->config.show_charts = false; // No charts for monitoring
    dashboard->config.show_alerts = true;
    dashboard->config.chart_history_size = 100;
    dashboard->config.warning_threshold_percent = 75.0f;
    dashboard->config.critical_threshold_percent = 85.0f;

    return 0;
}
