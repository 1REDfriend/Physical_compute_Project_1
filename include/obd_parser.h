#ifndef OBD_PARSER_H
#define OBD_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// OBD-II Protocol Types
typedef enum {
    OBD_PROTOCOL_ISO_9141_2 = 0x01,
    OBD_PROTOCOL_ISO_14230_4_KWP2000 = 0x02,
    OBD_PROTOCOL_ISO_15765_4_CAN = 0x03,
    OBD_PROTOCOL_UNKNOWN = 0xFF
} obd_protocol_t;

// OBD-II Service Modes
typedef enum {
    OBD_MODE_01_LIVE_DATA = 0x01,
    OBD_MODE_02_FREEZE_FRAME = 0x02,
    OBD_MODE_03_DTCS = 0x03,
    OBD_MODE_04_CLEAR_DTCS = 0x04,
    OBD_MODE_05_O2_SENSOR_TEST = 0x05,
    OBD_MODE_06_TEST_RESULTS = 0x06,
    OBD_MODE_07_PENDING_DTCS = 0x07,
    OBD_MODE_08_CONTROL_ONBOARD = 0x08,
    OBD_MODE_09_VEHICLE_INFO = 0x09,
    OBD_MODE_0A_PERMANENT_DTCS = 0x0A
} obd_mode_t;

// OBD-II Response Status
typedef enum {
    OBD_RESPONSE_OK = 0x00,
    OBD_RESPONSE_GENERAL_REJECT = 0x10,
    OBD_RESPONSE_SERVICE_NOT_SUPPORTED = 0x11,
    OBD_RESPONSE_SUB_FUNCTION_NOT_SUPPORTED = 0x12,
    OBD_RESPONSE_INCORRECT_MESSAGE_LENGTH = 0x13,
    OBD_RESPONSE_RESPONSE_TOO_LONG = 0x14,
    OBD_RESPONSE_BUSY_REPEAT_REQUEST = 0x21,
    OBD_RESPONSE_CONDITIONS_NOT_CORRECT = 0x22,
    OBD_RESPONSE_REQUEST_SEQUENCE_ERROR = 0x24,
    OBD_RESPONSE_NO_RESPONSE_FROM_SUBNET = 0x25,
    OBD_RESPONSE_FAILURE_PREVENTS_EXECUTION = 0x26,
    OBD_RESPONSE_REQUEST_OUT_OF_RANGE = 0x31,
    OBD_RESPONSE_SECURITY_ACCESS_DENIED = 0x33,
    OBD_RESPONSE_INVALID_KEY = 0x35,
    OBD_RESPONSE_EXCEED_NUMBER_OF_ATTEMPTS = 0x36,
    OBD_RESPONSE_REQUIRED_TIME_DELAY_NOT_EXPIRED = 0x37
} obd_response_status_t;

// OBD-II Message Structure
typedef struct {
    uint8_t* data;
    size_t length;
    uint64_t timestamp;
    obd_protocol_t protocol;
    bool is_request;
    bool is_response;
} obd_message_t;

// OBD-II Parsed Data
typedef struct {
    obd_mode_t mode;
    uint8_t pid;
    uint8_t* payload;
    size_t payload_length;
    obd_response_status_t status;
    bool is_multiframe;
    uint8_t frame_number;
    uint8_t total_frames;
    bool is_request;
    bool is_response;
    uint64_t timestamp;
} obd_parsed_data_t;

// OBD-II Parser Context
typedef struct {
    obd_protocol_t current_protocol;
    bool protocol_detected;
    uint8_t* buffer;
    size_t buffer_size;
    size_t buffer_pos;
    uint64_t last_activity;
    bool connection_active;
} obd_parser_t;

// Function prototypes
obd_parser_t* obd_parser_create(size_t buffer_size);
void obd_parser_destroy(obd_parser_t* parser);
int obd_parser_init(obd_parser_t* parser);

// Protocol detection
obd_protocol_t obd_detect_protocol(const uint8_t* data, size_t length);
bool obd_set_protocol(obd_parser_t* parser, obd_protocol_t protocol);

// Message parsing
int obd_parse_message(obd_parser_t* parser, const uint8_t* data, size_t length, obd_parsed_data_t* result);
int obd_parse_k_line_message(const uint8_t* data, size_t length, obd_parsed_data_t* result);
int obd_parse_can_message(const uint8_t* data, size_t length, obd_parsed_data_t* result);

// Message construction
int obd_build_request(obd_mode_t mode, uint8_t pid, uint8_t* output, size_t max_length);
int obd_build_k_line_request(obd_mode_t mode, uint8_t pid, uint8_t* output, size_t max_length);
int obd_build_can_request(obd_mode_t mode, uint8_t pid, uint8_t* output, size_t max_length);

// Utility functions
const char* obd_protocol_to_string(obd_protocol_t protocol);
const char* obd_mode_to_string(obd_mode_t mode);
const char* obd_response_status_to_string(obd_response_status_t status);
bool obd_is_valid_pid(obd_mode_t mode, uint8_t pid);

// Data processing
int obd_process_live_data(const obd_parsed_data_t* data, float* value, const char** unit);
int obd_process_dtc_codes(const obd_parsed_data_t* data, uint16_t* dtc_codes, size_t max_codes, size_t* code_count);

#ifdef __cplusplus
}
#endif

#endif // OBD_PARSER_H
