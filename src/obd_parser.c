#include "obd_parser.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// OBD-II Parser Implementation
obd_parser_t *obd_parser_create(size_t buffer_size)
{
    obd_parser_t *parser = (obd_parser_t *)calloc(1, sizeof(obd_parser_t));
    if (!parser)
        return NULL;

    parser->buffer = (uint8_t *)malloc(buffer_size);
    if (!parser->buffer)
    {
        free(parser);
        return NULL;
    }

    parser->buffer_size = buffer_size;
    parser->current_protocol = OBD_PROTOCOL_UNKNOWN;
    parser->protocol_detected = false;
    parser->connection_active = false;

    return parser;
}

void obd_parser_destroy(obd_parser_t *parser)
{
    if (!parser)
        return;

    if (parser->buffer)
    {
        free(parser->buffer);
    }
    free(parser);
}

// Protocol detection based on initial communication
obd_protocol_t obd_detect_protocol(const uint8_t *data, size_t length)
{
    if (!data || length == 0)
        return OBD_PROTOCOL_UNKNOWN;

    // ISO 9141-2 detection (5 baud init)
    if (length >= 1 && data[0] == 0x33)
    {
        return OBD_PROTOCOL_ISO_9141_2;
    }

    // ISO 14230-4 KWP2000 detection
    if (length >= 2 && data[0] == 0x81 && data[1] == 0x8F)
    {
        return OBD_PROTOCOL_ISO_14230_4_KWP2000;
    }

    // ISO 15765-4 CAN detection (29-bit or 11-bit identifier)
    if (length >= 4)
    {
        // Check for CAN frame structure
        uint32_t can_id = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        if ((can_id & 0x1FFFFFFF) == 0x7DF || (can_id & 0x7FF) == 0x7DF)
        {
            return OBD_PROTOCOL_ISO_15765_4_CAN;
        }
    }

    return OBD_PROTOCOL_UNKNOWN;
}

bool obd_set_protocol(obd_parser_t *parser, obd_protocol_t protocol)
{
    if (!parser)
        return false;

    parser->current_protocol = protocol;
    parser->protocol_detected = (protocol != OBD_PROTOCOL_UNKNOWN);
    return true;
}

// Parse OBD-II message based on protocol
int obd_parse_message(obd_parser_t *parser, const uint8_t *data, size_t length, obd_parsed_data_t *result)
{
    if (!parser || !data || !result)
        return -1;

    memset(result, 0, sizeof(obd_parsed_data_t));

    switch (parser->current_protocol)
    {
    case OBD_PROTOCOL_ISO_9141_2:
    case OBD_PROTOCOL_ISO_14230_4_KWP2000:
        return obd_parse_k_line_message(data, length, result);

    case OBD_PROTOCOL_ISO_15765_4_CAN:
        return obd_parse_can_message(data, length, result);

    default:
        return -1;
    }
}

// Parse K-Line message (ISO 9141-2, ISO 14230-4)
int obd_parse_k_line_message(const uint8_t *data, size_t length, obd_parsed_data_t *result)
{
    if (!data || length < 3 || !result)
        return -1;

    // K-Line message format: [Header] [Mode] [PID] [Data...] [Checksum]
    result->mode = (obd_mode_t)data[1];
    result->pid = data[2];

    // Check if it's a response (starts with 0x40 + mode)
    if (data[0] == 0x40 + data[1])
    {
        result->is_response = true;
        result->is_request = false;

        // Parse payload (skip header, mode, pid)
        if (length > 3)
        {
            result->payload_length = length - 4; // -4 for header, mode, pid, checksum
            if (result->payload_length > 0)
            {
                result->payload = (uint8_t *)malloc(result->payload_length);
                if (result->payload)
                {
                    memcpy(result->payload, &data[3], result->payload_length);
                }
            }
        }

        // Check for error response
        if (result->payload_length >= 1)
        {
            result->status = (obd_response_status_t)result->payload[0];
        }
    }
    else
    {
        result->is_request = true;
        result->is_response = false;
    }

    return 0;
}

// Parse CAN message (ISO 15765-4)
int obd_parse_can_message(const uint8_t *data, size_t length, obd_parsed_data_t *result)
{
    if (!data || length < 8 || !result)
        return -1;

    // CAN message format: [ID] [DLC] [Data...]
    // OBD-II CAN uses 0x7DF for requests, 0x7E8-0x7EF for responses

    uint32_t can_id = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    uint8_t dlc = data[4];

    if (can_id == 0x7DF)
    {
        // Request message
        result->is_request = true;
        result->is_response = false;
        result->mode = (obd_mode_t)data[5];
        result->pid = data[6];
    }
    else if (can_id >= 0x7E8 && can_id <= 0x7EF)
    {
        // Response message
        result->is_response = true;
        result->is_request = false;
        result->mode = (obd_mode_t)data[5];
        result->pid = data[6];

        // Parse payload
        if (dlc > 2)
        {
            result->payload_length = dlc - 2; // -2 for mode and pid
            if (result->payload_length > 0)
            {
                result->payload = (uint8_t *)malloc(result->payload_length);
                if (result->payload)
                {
                    memcpy(result->payload, &data[7], result->payload_length);
                }
            }
        }
    }

    return 0;
}

// Build OBD-II request message
int obd_build_request(obd_mode_t mode, uint8_t pid, uint8_t *output, size_t max_length)
{
    if (!output || max_length < 4)
        return -1;

    output[0] = 0x33; // Header
    output[1] = (uint8_t)mode;
    output[2] = pid;

    // Calculate checksum (simple XOR)
    uint8_t checksum = output[0] ^ output[1] ^ output[2];
    output[3] = checksum;

    return 4;
}

// Build K-Line request
int obd_build_k_line_request(obd_mode_t mode, uint8_t pid, uint8_t *output, size_t max_length)
{
    return obd_build_request(mode, pid, output, max_length);
}

// Build CAN request
int obd_build_can_request(obd_mode_t mode, uint8_t pid, uint8_t *output, size_t max_length)
{
    if (!output || max_length < 8)
        return -1;

    // CAN ID for OBD-II requests
    output[0] = 0x00;
    output[1] = 0x00;
    output[2] = 0x07;
    output[3] = 0xDF;

    // DLC (Data Length Code)
    output[4] = 0x03; // 3 bytes: mode, pid, data

    // Mode and PID
    output[5] = (uint8_t)mode;
    output[6] = pid;
    output[7] = 0x00; // No additional data

    return 8;
}

// Utility functions
const char *obd_protocol_to_string(obd_protocol_t protocol)
{
    switch (protocol)
    {
    case OBD_PROTOCOL_ISO_9141_2:
        return "ISO 9141-2";
    case OBD_PROTOCOL_ISO_14230_4_KWP2000:
        return "ISO 14230-4 KWP2000";
    case OBD_PROTOCOL_ISO_15765_4_CAN:
        return "ISO 15765-4 CAN";
    default:
        return "Unknown";
    }
}

const char *obd_mode_to_string(obd_mode_t mode)
{
    switch (mode)
    {
    case OBD_MODE_01_LIVE_DATA:
        return "Live Data";
    case OBD_MODE_02_FREEZE_FRAME:
        return "Freeze Frame";
    case OBD_MODE_03_DTCS:
        return "DTCs";
    case OBD_MODE_04_CLEAR_DTCS:
        return "Clear DTCs";
    case OBD_MODE_05_O2_SENSOR_TEST:
        return "O2 Sensor Test";
    case OBD_MODE_06_TEST_RESULTS:
        return "Test Results";
    case OBD_MODE_07_PENDING_DTCS:
        return "Pending DTCs";
    case OBD_MODE_08_CONTROL_ONBOARD:
        return "Control Onboard";
    case OBD_MODE_09_VEHICLE_INFO:
        return "Vehicle Info";
    case OBD_MODE_0A_PERMANENT_DTCS:
        return "Permanent DTCs";
    default:
        return "Unknown Mode";
    }
}

const char *obd_response_status_to_string(obd_response_status_t status)
{
    switch (status)
    {
    case OBD_RESPONSE_OK:
        return "OK";
    case OBD_RESPONSE_GENERAL_REJECT:
        return "General Reject";
    case OBD_RESPONSE_SERVICE_NOT_SUPPORTED:
        return "Service Not Supported";
    case OBD_RESPONSE_SUB_FUNCTION_NOT_SUPPORTED:
        return "Sub Function Not Supported";
    case OBD_RESPONSE_INCORRECT_MESSAGE_LENGTH:
        return "Incorrect Message Length";
    case OBD_RESPONSE_RESPONSE_TOO_LONG:
        return "Response Too Long";
    case OBD_RESPONSE_BUSY_REPEAT_REQUEST:
        return "Busy Repeat Request";
    case OBD_RESPONSE_CONDITIONS_NOT_CORRECT:
        return "Conditions Not Correct";
    case OBD_RESPONSE_REQUEST_SEQUENCE_ERROR:
        return "Request Sequence Error";
    case OBD_RESPONSE_NO_RESPONSE_FROM_SUBNET:
        return "No Response From Subnet";
    case OBD_RESPONSE_FAILURE_PREVENTS_EXECUTION:
        return "Failure Prevents Execution";
    case OBD_RESPONSE_REQUEST_OUT_OF_RANGE:
        return "Request Out Of Range";
    case OBD_RESPONSE_SECURITY_ACCESS_DENIED:
        return "Security Access Denied";
    case OBD_RESPONSE_INVALID_KEY:
        return "Invalid Key";
    case OBD_RESPONSE_EXCEED_NUMBER_OF_ATTEMPTS:
        return "Exceed Number Of Attempts";
    case OBD_RESPONSE_REQUIRED_TIME_DELAY_NOT_EXPIRED:
        return "Required Time Delay Not Expired";
    default:
        return "Unknown Status";
    }
}

bool obd_is_valid_pid(obd_mode_t mode, uint8_t pid)
{
    switch (mode)
    {
    case OBD_MODE_01_LIVE_DATA:
        return (pid >= 0x00 && pid <= 0x4E);
    case OBD_MODE_02_FREEZE_FRAME:
        return (pid >= 0x00 && pid <= 0x4E);
    case OBD_MODE_03_DTCS:
    case OBD_MODE_07_PENDING_DTCS:
    case OBD_MODE_0A_PERMANENT_DTCS:
        return true; // No specific PID for DTC requests
    default:
        return false;
    }
}

// Process live data values
int obd_process_live_data(const obd_parsed_data_t *data, float *value, const char **unit)
{
    if (!data || !value || !unit)
        return -1;

    if (data->mode != OBD_MODE_01_LIVE_DATA || !data->payload || data->payload_length < 2)
    {
        return -1;
    }

    // Basic PID processing (simplified)
    switch (data->pid)
    {
    case 0x0C: // Engine RPM
        *value = ((data->payload[0] * 256) + data->payload[1]) / 4.0f;
        *unit = "RPM";
        break;
    case 0x0D: // Vehicle Speed
        *value = (float)data->payload[0];
        *unit = "km/h";
        break;
    case 0x05: // Engine Coolant Temperature
        *value = data->payload[0] - 40.0f;
        *unit = "°C";
        break;
    case 0x0F: // Intake Air Temperature
        *value = data->payload[0] - 40.0f;
        *unit = "°C";
        break;
    default:
        return -1;
    }

    return 0;
}

// Process DTC codes
int obd_process_dtc_codes(const obd_parsed_data_t *data, uint16_t *dtc_codes, size_t max_codes, size_t *code_count)
{
    if (!data || !dtc_codes || !code_count)
        return -1;

    if (data->mode != OBD_MODE_03_DTCS && data->mode != OBD_MODE_07_PENDING_DTCS)
    {
        return -1;
    }

    *code_count = 0;

    if (!data->payload || data->payload_length < 2)
    {
        return 0;
    }

    // DTC format: [A][B][C][D] where DTC = (A<<12) | (B<<8) | (C<<4) | D
    size_t dtc_bytes = data->payload_length;
    size_t max_dtcs = dtc_bytes / 2;

    if (max_dtcs > max_codes)
        max_dtcs = max_codes;

    for (size_t i = 0; i < max_dtcs; i++)
    {
        uint8_t byte1 = data->payload[i * 2];
        uint8_t byte2 = data->payload[i * 2 + 1];

        // Convert to DTC code
        uint16_t dtc = ((byte1 & 0xC0) << 2) | ((byte1 & 0x3F) << 8) | byte2;
        dtc_codes[*code_count] = dtc;
        (*code_count)++;
    }

    return 0;
}

// Initialize OBD parser
int obd_parser_init(obd_parser_t* parser)
{
    if (!parser)
        return -1;
    
    // Initialize parser state
    parser->buffer_size = parser->buffer_size; // Already set in create
    parser->buffer = parser->buffer; // Already allocated in create
    
    return 0;
}
