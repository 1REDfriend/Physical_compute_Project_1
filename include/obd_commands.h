#ifndef OBD_COMMANDS_H
#define OBD_COMMANDS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// OBD-II PID Definitions
typedef enum {
    // Mode 01 - Live Data PIDs
    OBD_PID_ENGINE_LOAD = 0x04,
    OBD_PID_COOLANT_TEMP = 0x05,
    OBD_PID_SHORT_TERM_FUEL_TRIM_1 = 0x06,
    OBD_PID_LONG_TERM_FUEL_TRIM_1 = 0x07,
    OBD_PID_SHORT_TERM_FUEL_TRIM_2 = 0x08,
    OBD_PID_LONG_TERM_FUEL_TRIM_2 = 0x09,
    OBD_PID_FUEL_PRESSURE = 0x0A,
    OBD_PID_INTAKE_MAP = 0x0B,
    OBD_PID_ENGINE_RPM = 0x0C,
    OBD_PID_VEHICLE_SPEED = 0x0D,
    OBD_PID_TIMING_ADVANCE = 0x0E,
    OBD_PID_INTAKE_AIR_TEMP = 0x0F,
    OBD_PID_MAF_AIR_FLOW = 0x10,
    OBD_PID_THROTTLE_POSITION = 0x11,
    OBD_PID_O2_SENSOR_1 = 0x14,
    OBD_PID_O2_SENSOR_2 = 0x15,
    OBD_PID_O2_SENSOR_3 = 0x16,
    OBD_PID_O2_SENSOR_4 = 0x17,
    OBD_PID_O2_SENSOR_5 = 0x18,
    OBD_PID_O2_SENSOR_6 = 0x19,
    OBD_PID_O2_SENSOR_7 = 0x1A,
    OBD_PID_O2_SENSOR_8 = 0x1B,
    OBD_PID_FUEL_TANK_LEVEL = 0x2F,
    OBD_PID_AMBIENT_AIR_TEMP = 0x46,
    OBD_PID_ENGINE_FUEL_RATE = 0x5E
} obd_pid_t;

// Vehicle Manufacturer Codes
typedef enum {
    VEHICLE_MAKE_GENERIC = 0x00,
    VEHICLE_MAKE_TOYOTA = 0x01,
    VEHICLE_MAKE_HONDA = 0x02,
    VEHICLE_MAKE_FORD = 0x03,
    VEHICLE_MAKE_GM = 0x04,
    VEHICLE_MAKE_CHRYSLER = 0x05,
    VEHICLE_MAKE_NISSAN = 0x06,
    VEHICLE_MAKE_HYUNDAI = 0x07,
    VEHICLE_MAKE_VW = 0x08,
    VEHICLE_MAKE_BMW = 0x09,
    VEHICLE_MAKE_MERCEDES = 0x0A
} vehicle_make_t;

// OBD-II Command Structure
typedef struct {
    uint8_t mode;
    uint8_t pid;
    const char* name;
    const char* unit;
    float min_value;
    float max_value;
    bool is_supported;
} obd_command_t;

// OBD-II Command Library
typedef struct {
    obd_command_t* commands;
    size_t command_count;
    vehicle_make_t vehicle_make;
    bool is_initialized;
} obd_command_library_t;

// Function prototypes
obd_command_library_t* obd_command_library_create(void);
void obd_command_library_destroy(obd_command_library_t* library);

// Library management
int obd_command_library_init(obd_command_library_t* library, vehicle_make_t make);
int obd_command_library_set_vehicle_make(obd_command_library_t* library, vehicle_make_t make);

// Command queries
const obd_command_t* obd_command_library_get_command(obd_command_library_t* library, uint8_t mode, uint8_t pid);
const obd_command_t* obd_command_library_get_command_by_name(obd_command_library_t* library, const char* name);
size_t obd_command_library_get_supported_commands(obd_command_library_t* library, obd_command_t* commands, size_t max_commands);

// PID support checking
bool obd_command_library_is_pid_supported(obd_command_library_t* library, uint8_t mode, uint8_t pid);
int obd_command_library_set_pid_support(obd_command_library_t* library, uint8_t mode, uint8_t pid, bool supported);

// Common OBD-II commands
int obd_command_library_get_engine_rpm(obd_command_library_t* library, obd_command_t* command);
int obd_command_library_get_vehicle_speed(obd_command_library_t* library, obd_command_t* command);
int obd_command_library_get_coolant_temp(obd_command_library_t* library, obd_command_t* command);
int obd_command_library_get_intake_air_temp(obd_command_library_t* library, obd_command_t* command);
int obd_command_library_get_throttle_position(obd_command_library_t* library, obd_command_t* command);
int obd_command_library_get_fuel_level(obd_command_library_t* library, obd_command_t* command);

// DTC commands
int obd_command_library_get_dtc_codes(obd_command_library_t* library, obd_command_t* command);
int obd_command_library_get_pending_dtc_codes(obd_command_library_t* library, obd_command_t* command);
int obd_command_library_clear_dtc_codes(obd_command_library_t* library, obd_command_t* command);

// Utility functions
const char* obd_pid_to_string(obd_pid_t pid);
const char* obd_pid_to_name(obd_pid_t pid);
const char* vehicle_make_to_string(vehicle_make_t make);
float obd_parse_pid_value(const uint8_t* data, size_t length, obd_pid_t pid);

#ifdef __cplusplus
}
#endif

#endif // OBD_COMMANDS_H
