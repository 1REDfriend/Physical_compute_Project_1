#include "obd_commands.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Pre-defined OBD-II commands for different vehicle makes
static const obd_command_t toyota_commands[] = {
    {0x01, 0x04, "Engine Load", "%", 0.0f, 100.0f, true},
    {0x01, 0x05, "Coolant Temperature", "°C", -40.0f, 215.0f, true},
    {0x01, 0x0C, "Engine RPM", "RPM", 0.0f, 8000.0f, true},
    {0x01, 0x0D, "Vehicle Speed", "km/h", 0.0f, 255.0f, true},
    {0x01, 0x0F, "Intake Air Temperature", "°C", -40.0f, 215.0f, true},
    {0x01, 0x11, "Throttle Position", "%", 0.0f, 100.0f, true},
    {0x01, 0x2F, "Fuel Tank Level", "%", 0.0f, 100.0f, true},
    {0x01, 0x46, "Ambient Air Temperature", "°C", -40.0f, 215.0f, true},
    {0x03, 0x00, "DTC Codes", "Codes", 0.0f, 0.0f, true},
    {0x07, 0x00, "Pending DTC Codes", "Codes", 0.0f, 0.0f, true},
    {0x04, 0x00, "Clear DTC Codes", "Status", 0.0f, 0.0f, true}};

static const obd_command_t honda_commands[] = {
    {0x01, 0x04, "Engine Load", "%", 0.0f, 100.0f, true},
    {0x01, 0x05, "Coolant Temperature", "°C", -40.0f, 215.0f, true},
    {0x01, 0x0C, "Engine RPM", "RPM", 0.0f, 8000.0f, true},
    {0x01, 0x0D, "Vehicle Speed", "km/h", 0.0f, 255.0f, true},
    {0x01, 0x0F, "Intake Air Temperature", "°C", -40.0f, 215.0f, true},
    {0x01, 0x11, "Throttle Position", "%", 0.0f, 100.0f, true},
    {0x01, 0x2F, "Fuel Tank Level", "%", 0.0f, 100.0f, true},
    {0x01, 0x46, "Ambient Air Temperature", "°C", -40.0f, 215.0f, true},
    {0x03, 0x00, "DTC Codes", "Codes", 0.0f, 0.0f, true},
    {0x07, 0x00, "Pending DTC Codes", "Codes", 0.0f, 0.0f, true},
    {0x04, 0x00, "Clear DTC Codes", "Status", 0.0f, 0.0f, true}};

static const obd_command_t ford_commands[] = {
    {0x01, 0x04, "Engine Load", "%", 0.0f, 100.0f, true},
    {0x01, 0x05, "Coolant Temperature", "°C", -40.0f, 215.0f, true},
    {0x01, 0x0C, "Engine RPM", "RPM", 0.0f, 8000.0f, true},
    {0x01, 0x0D, "Vehicle Speed", "km/h", 0.0f, 255.0f, true},
    {0x01, 0x0F, "Intake Air Temperature", "°C", -40.0f, 215.0f, true},
    {0x01, 0x11, "Throttle Position", "%", 0.0f, 100.0f, true},
    {0x01, 0x2F, "Fuel Tank Level", "%", 0.0f, 100.0f, true},
    {0x01, 0x46, "Ambient Air Temperature", "°C", -40.0f, 215.0f, true},
    {0x03, 0x00, "DTC Codes", "Codes", 0.0f, 0.0f, true},
    {0x07, 0x00, "Pending DTC Codes", "Codes", 0.0f, 0.0f, true},
    {0x04, 0x00, "Clear DTC Codes", "Status", 0.0f, 0.0f, true}};

static const obd_command_t generic_commands[] = {
    {0x01, 0x04, "Engine Load", "%", 0.0f, 100.0f, true},
    {0x01, 0x05, "Coolant Temperature", "°C", -40.0f, 215.0f, true},
    {0x01, 0x0C, "Engine RPM", "RPM", 0.0f, 8000.0f, true},
    {0x01, 0x0D, "Vehicle Speed", "km/h", 0.0f, 255.0f, true},
    {0x01, 0x0F, "Intake Air Temperature", "°C", -40.0f, 215.0f, true},
    {0x01, 0x11, "Throttle Position", "%", 0.0f, 100.0f, true},
    {0x01, 0x2F, "Fuel Tank Level", "%", 0.0f, 100.0f, true},
    {0x01, 0x46, "Ambient Air Temperature", "°C", -40.0f, 215.0f, true},
    {0x03, 0x00, "DTC Codes", "Codes", 0.0f, 0.0f, true},
    {0x07, 0x00, "Pending DTC Codes", "Codes", 0.0f, 0.0f, true},
    {0x04, 0x00, "Clear DTC Codes", "Status", 0.0f, 0.0f, true}};

// OBD-II Command Library Implementation
obd_command_library_t *obd_command_library_create(void)
{
    obd_command_library_t *library = (obd_command_library_t *)calloc(1, sizeof(obd_command_library_t));
    if (!library)
        return NULL;

    library->commands = NULL;
    library->command_count = 0;
    library->vehicle_make = VEHICLE_MAKE_GENERIC;
    library->is_initialized = false;

    return library;
}

void obd_command_library_destroy(obd_command_library_t *library)
{
    if (!library)
        return;

    if (library->commands)
    {
        free(library->commands);
    }
    free(library);
}

int obd_command_library_init(obd_command_library_t *library, vehicle_make_t make)
{
    if (!library)
        return -1;

    const obd_command_t *source_commands = NULL;
    size_t source_count = 0;

    switch (make)
    {
    case VEHICLE_MAKE_TOYOTA:
        source_commands = toyota_commands;
        source_count = sizeof(toyota_commands) / sizeof(toyota_commands[0]);
        break;
    case VEHICLE_MAKE_HONDA:
        source_commands = honda_commands;
        source_count = sizeof(honda_commands) / sizeof(honda_commands[0]);
        break;
    case VEHICLE_MAKE_FORD:
        source_commands = ford_commands;
        source_count = sizeof(ford_commands) / sizeof(ford_commands[0]);
        break;
    default:
        source_commands = generic_commands;
        source_count = sizeof(generic_commands) / sizeof(generic_commands[0]);
        break;
    }

    if (library->commands)
    {
        free(library->commands);
    }

    library->commands = (obd_command_t *)malloc(source_count * sizeof(obd_command_t));
    if (!library->commands)
        return -1;

    memcpy(library->commands, source_commands, source_count * sizeof(obd_command_t));
    library->command_count = source_count;
    library->vehicle_make = make;
    library->is_initialized = true;

    return 0;
}

int obd_command_library_set_vehicle_make(obd_command_library_t *library, vehicle_make_t make)
{
    if (!library)
        return -1;

    return obd_command_library_init(library, make);
}

const obd_command_t *obd_command_library_get_command(obd_command_library_t *library, uint8_t mode, uint8_t pid)
{
    if (!library || !library->is_initialized)
        return NULL;

    for (size_t i = 0; i < library->command_count; i++)
    {
        if (library->commands[i].mode == mode && library->commands[i].pid == pid)
        {
            return &library->commands[i];
        }
    }

    return NULL;
}

const obd_command_t *obd_command_library_get_command_by_name(obd_command_library_t *library, const char *name)
{
    if (!library || !library->is_initialized || !name)
        return NULL;

    for (size_t i = 0; i < library->command_count; i++)
    {
        if (strcmp(library->commands[i].name, name) == 0)
        {
            return &library->commands[i];
        }
    }

    return NULL;
}

size_t obd_command_library_get_supported_commands(obd_command_library_t *library, obd_command_t *commands, size_t max_commands)
{
    if (!library || !library->is_initialized || !commands)
        return 0;

    size_t count = 0;
    for (size_t i = 0; i < library->command_count && count < max_commands; i++)
    {
        if (library->commands[i].is_supported)
        {
            commands[count] = library->commands[i];
            count++;
        }
    }

    return count;
}

bool obd_command_library_is_pid_supported(obd_command_library_t *library, uint8_t mode, uint8_t pid)
{
    const obd_command_t *command = obd_command_library_get_command(library, mode, pid);
    return command ? command->is_supported : false;
}

int obd_command_library_set_pid_support(obd_command_library_t *library, uint8_t mode, uint8_t pid, bool supported)
{
    if (!library || !library->is_initialized)
        return -1;

    for (size_t i = 0; i < library->command_count; i++)
    {
        if (library->commands[i].mode == mode && library->commands[i].pid == pid)
        {
            library->commands[i].is_supported = supported;
            return 0;
        }
    }

    return -1; // Command not found
}

// Common OBD-II commands
int obd_command_library_get_engine_rpm(obd_command_library_t *library, obd_command_t *command)
{
    if (!library || !command)
        return -1;

    const obd_command_t *rpm_cmd = obd_command_library_get_command(library, 0x01, 0x0C);
    if (!rpm_cmd)
        return -1;

    *command = *rpm_cmd;
    return 0;
}

int obd_command_library_get_vehicle_speed(obd_command_library_t *library, obd_command_t *command)
{
    if (!library || !command)
        return -1;

    const obd_command_t *speed_cmd = obd_command_library_get_command(library, 0x01, 0x0D);
    if (!speed_cmd)
        return -1;

    *command = *speed_cmd;
    return 0;
}

int obd_command_library_get_coolant_temp(obd_command_library_t *library, obd_command_t *command)
{
    if (!library || !command)
        return -1;

    const obd_command_t *temp_cmd = obd_command_library_get_command(library, 0x01, 0x05);
    if (!temp_cmd)
        return -1;

    *command = *temp_cmd;
    return 0;
}

int obd_command_library_get_intake_air_temp(obd_command_library_t *library, obd_command_t *command)
{
    if (!library || !command)
        return -1;

    const obd_command_t *temp_cmd = obd_command_library_get_command(library, 0x01, 0x0F);
    if (!temp_cmd)
        return -1;

    *command = *temp_cmd;
    return 0;
}

int obd_command_library_get_throttle_position(obd_command_library_t *library, obd_command_t *command)
{
    if (!library || !command)
        return -1;

    const obd_command_t *throttle_cmd = obd_command_library_get_command(library, 0x01, 0x11);
    if (!throttle_cmd)
        return -1;

    *command = *throttle_cmd;
    return 0;
}

int obd_command_library_get_fuel_level(obd_command_library_t *library, obd_command_t *command)
{
    if (!library || !command)
        return -1;

    const obd_command_t *fuel_cmd = obd_command_library_get_command(library, 0x01, 0x2F);
    if (!fuel_cmd)
        return -1;

    *command = *fuel_cmd;
    return 0;
}

// DTC commands
int obd_command_library_get_dtc_codes(obd_command_library_t *library, obd_command_t *command)
{
    if (!library || !command)
        return -1;

    const obd_command_t *dtc_cmd = obd_command_library_get_command(library, 0x03, 0x00);
    if (!dtc_cmd)
        return -1;

    *command = *dtc_cmd;
    return 0;
}

int obd_command_library_get_pending_dtc_codes(obd_command_library_t *library, obd_command_t *command)
{
    if (!library || !command)
        return -1;

    const obd_command_t *pending_cmd = obd_command_library_get_command(library, 0x07, 0x00);
    if (!pending_cmd)
        return -1;

    *command = *pending_cmd;
    return 0;
}

int obd_command_library_clear_dtc_codes(obd_command_library_t *library, obd_command_t *command)
{
    if (!library || !command)
        return -1;

    const obd_command_t *clear_cmd = obd_command_library_get_command(library, 0x04, 0x00);
    if (!clear_cmd)
        return -1;

    *command = *clear_cmd;
    return 0;
}

// Utility functions
const char *obd_pid_to_string(obd_pid_t pid)
{
    switch (pid)
    {
    case OBD_PID_ENGINE_LOAD:
        return "Engine Load";
    case OBD_PID_COOLANT_TEMP:
        return "Coolant Temperature";
    case OBD_PID_ENGINE_RPM:
        return "Engine RPM";
    case OBD_PID_VEHICLE_SPEED:
        return "Vehicle Speed";
    case OBD_PID_INTAKE_AIR_TEMP:
        return "Intake Air Temperature";
    case OBD_PID_THROTTLE_POSITION:
        return "Throttle Position";
    case OBD_PID_FUEL_TANK_LEVEL:
        return "Fuel Tank Level";
    case OBD_PID_AMBIENT_AIR_TEMP:
        return "Ambient Air Temperature";
    default:
        return "Unknown PID";
    }
}

const char *obd_pid_to_name(obd_pid_t pid)
{
    return obd_pid_to_string(pid);
}

const char *vehicle_make_to_string(vehicle_make_t make)
{
    switch (make)
    {
    case VEHICLE_MAKE_GENERIC:
        return "Generic";
    case VEHICLE_MAKE_TOYOTA:
        return "Toyota";
    case VEHICLE_MAKE_HONDA:
        return "Honda";
    case VEHICLE_MAKE_FORD:
        return "Ford";
    case VEHICLE_MAKE_GM:
        return "General Motors";
    case VEHICLE_MAKE_CHRYSLER:
        return "Chrysler";
    case VEHICLE_MAKE_NISSAN:
        return "Nissan";
    case VEHICLE_MAKE_HYUNDAI:
        return "Hyundai";
    case VEHICLE_MAKE_VW:
        return "Volkswagen";
    case VEHICLE_MAKE_BMW:
        return "BMW";
    case VEHICLE_MAKE_MERCEDES:
        return "Mercedes-Benz";
    default:
        return "Unknown";
    }
}

float obd_parse_pid_value(const uint8_t *data, size_t length, obd_pid_t pid)
{
    if (!data || length < 2)
        return 0.0f;

    switch (pid)
    {
    case OBD_PID_ENGINE_LOAD:
        return (data[0] * 100.0f) / 255.0f;

    case OBD_PID_COOLANT_TEMP:
    case OBD_PID_INTAKE_AIR_TEMP:
    case OBD_PID_AMBIENT_AIR_TEMP:
        return data[0] - 40.0f;

    case OBD_PID_ENGINE_RPM:
        return ((data[0] * 256) + data[1]) / 4.0f;

    case OBD_PID_VEHICLE_SPEED:
        return (float)data[0];

    case OBD_PID_THROTTLE_POSITION:
    case OBD_PID_FUEL_TANK_LEVEL:
        return (data[0] * 100.0f) / 255.0f;

    default:
        return 0.0f;
    }
}
