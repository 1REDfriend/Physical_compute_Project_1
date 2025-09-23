#include "dtc_viewer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// DTC Database - Common DTC codes
static const dtc_database_entry_t common_dtc_database[] = {
    // Engine Misfire Codes
    {0x0301, "P0301", "Cylinder 1 Misfire Detected", "Faulty spark plug, ignition coil, or fuel injector", "Check and replace spark plug, ignition coil, or fuel injector", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_ERROR, {"Engine", "Ignition"}, 2},
    {0x0302, "P0302", "Cylinder 2 Misfire Detected", "Faulty spark plug, ignition coil, or fuel injector", "Check and replace spark plug, ignition coil, or fuel injector", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_ERROR, {"Engine", "Ignition"}, 2},
    {0x0303, "P0303", "Cylinder 3 Misfire Detected", "Faulty spark plug, ignition coil, or fuel injector", "Check and replace spark plug, ignition coil, or fuel injector", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_ERROR, {"Engine", "Ignition"}, 2},
    {0x0304, "P0304", "Cylinder 4 Misfire Detected", "Faulty spark plug, ignition coil, or fuel injector", "Check and replace spark plug, ignition coil, or fuel injector", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_ERROR, {"Engine", "Ignition"}, 2},

    // Fuel System Codes
    {0x0171, "P0171", "System Too Lean (Bank 1)", "Vacuum leak, faulty MAF sensor, or fuel delivery issue", "Check for vacuum leaks, test MAF sensor, check fuel pressure", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_WARNING, {"Engine", "Fuel System"}, 2},
    {0x0172, "P0172", "System Too Rich (Bank 1)", "Faulty oxygen sensor, fuel injector, or fuel pressure regulator", "Check oxygen sensor, fuel injectors, and fuel pressure", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_WARNING, {"Engine", "Fuel System"}, 2},

    // Oxygen Sensor Codes
    {0x0130, "P0130", "O2 Sensor Circuit Malfunction (Bank 1 Sensor 1)", "Faulty oxygen sensor or wiring", "Replace oxygen sensor and check wiring", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_ERROR, {"Engine", "Emission System"}, 2},
    {0x0131, "P0131", "O2 Sensor Circuit Low Voltage (Bank 1 Sensor 1)", "Faulty oxygen sensor or wiring", "Replace oxygen sensor and check wiring", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_ERROR, {"Engine", "Emission System"}, 2},
    {0x0132, "P0132", "O2 Sensor Circuit High Voltage (Bank 1 Sensor 1)", "Faulty oxygen sensor or wiring", "Replace oxygen sensor and check wiring", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_ERROR, {"Engine", "Emission System"}, 2},

    // Transmission Codes
    {0x0700, "P0700", "Transmission Control System Malfunction", "Transmission control module fault", "Check transmission control module and wiring", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_CRITICAL, {"Transmission"}, 1},
    {0x0701, "P0701", "Transmission Control System Range/Performance", "Transmission control system performance issue", "Check transmission control system", DTC_CATEGORY_POWERTRAIN, DTC_SEVERITY_ERROR, {"Transmission"}, 1},

    // ABS/Chassis Codes
    {0x1001, "C1001", "ABS System Malfunction", "ABS control module fault", "Check ABS control module and sensors", DTC_CATEGORY_CHASSIS, DTC_SEVERITY_CRITICAL, {"ABS", "Brakes"}, 2},
    {0x1002, "C1002", "ABS Wheel Speed Sensor Malfunction", "Faulty wheel speed sensor", "Replace wheel speed sensor", DTC_CATEGORY_CHASSIS, DTC_SEVERITY_ERROR, {"ABS", "Brakes"}, 2},

    // Body Codes
    {0x2001, "B2001", "Driver Door Ajar Circuit Malfunction", "Faulty door switch or wiring", "Check door switch and wiring", DTC_CATEGORY_BODY, DTC_SEVERITY_WARNING, {"Body", "Doors"}, 1},
    {0x2002, "B2002", "Passenger Door Ajar Circuit Malfunction", "Faulty door switch or wiring", "Check door switch and wiring", DTC_CATEGORY_BODY, DTC_SEVERITY_WARNING, {"Body", "Doors"}, 1},

    // Network Codes
    {0x3001, "U3001", "Control Module Power Supply Circuit", "Faulty power supply to control module", "Check power supply and wiring", DTC_CATEGORY_NETWORK, DTC_SEVERITY_CRITICAL, {"Network", "Power"}, 1},
    {0x3002, "U3002", "Control Module Ground Circuit", "Faulty ground connection", "Check ground connections", DTC_CATEGORY_NETWORK, DTC_SEVERITY_CRITICAL, {"Network", "Ground"}, 1}};

static const size_t common_dtc_database_size = sizeof(common_dtc_database) / sizeof(common_dtc_database[0]);

// DTC Viewer Implementation
dtc_viewer_t *dtc_viewer_create(void)
{
    dtc_viewer_t *viewer = (dtc_viewer_t *)calloc(1, sizeof(dtc_viewer_t));
    if (!viewer)
        return NULL;

    viewer->active_dtcs = NULL;
    viewer->pending_dtcs = NULL;
    viewer->permanent_dtcs = NULL;
    viewer->history = NULL;
    viewer->database = NULL;
    viewer->active_count = 0;
    viewer->pending_count = 0;
    viewer->permanent_count = 0;
    viewer->history_count = 0;
    viewer->database_size = 0;
    viewer->is_initialized = false;

    return viewer;
}

void dtc_viewer_destroy(dtc_viewer_t *viewer)
{
    if (!viewer)
        return;

    if (viewer->active_dtcs)
        free(viewer->active_dtcs);
    if (viewer->pending_dtcs)
        free(viewer->pending_dtcs);
    if (viewer->permanent_dtcs)
        free(viewer->permanent_dtcs);
    if (viewer->history)
        free(viewer->history);
    if (viewer->database)
        free(viewer->database);

    free(viewer);
}

int dtc_viewer_init(dtc_viewer_t *viewer)
{
    if (!viewer)
        return -1;

    // Initialize arrays
    viewer->active_dtcs = (dtc_code_t *)calloc(32, sizeof(dtc_code_t));
    viewer->pending_dtcs = (dtc_code_t *)calloc(32, sizeof(dtc_code_t));
    viewer->permanent_dtcs = (dtc_code_t *)calloc(32, sizeof(dtc_code_t));
    viewer->history = (dtc_history_entry_t *)calloc(1000, sizeof(dtc_history_entry_t));

    if (!viewer->active_dtcs || !viewer->pending_dtcs || !viewer->permanent_dtcs || !viewer->history)
    {
        return -1;
    }

    // Load common database
    viewer->database = (dtc_database_entry_t *)malloc(common_dtc_database_size * sizeof(dtc_database_entry_t));
    if (!viewer->database)
        return -1;

    memcpy(viewer->database, common_dtc_database, common_dtc_database_size * sizeof(dtc_database_entry_t));
    viewer->database_size = common_dtc_database_size;

    // Initialize statistics
    memset(&viewer->statistics, 0, sizeof(dtc_statistics_t));

    viewer->is_initialized = true;
    return 0;
}

int dtc_viewer_add_dtc(dtc_viewer_t *viewer, uint16_t code, bool is_active, bool is_pending, bool is_permanent)
{
    if (!viewer || !viewer->is_initialized)
        return -1;

    uint64_t current_time = time(NULL);

    // Check if DTC already exists
    dtc_code_t *existing_dtc = NULL;
    if (is_active)
    {
        for (size_t i = 0; i < viewer->active_count; i++)
        {
            if (viewer->active_dtcs[i].code == code)
            {
                existing_dtc = &viewer->active_dtcs[i];
                break;
            }
        }
    }

    if (existing_dtc)
    {
        // Update existing DTC
        existing_dtc->last_seen = current_time;
        existing_dtc->occurrence_count++;
    }
    else
    {
        // Create new DTC
        dtc_code_t new_dtc = {0};
        new_dtc.code = code;
        snprintf(new_dtc.code_string, sizeof(new_dtc.code_string), "%04X", code);
        new_dtc.first_seen = current_time;
        new_dtc.last_seen = current_time;
        new_dtc.occurrence_count = 1;
        new_dtc.is_active = is_active;
        new_dtc.is_pending = is_pending;
        new_dtc.is_permanent = is_permanent;

        // Get database entry for description
        const dtc_database_entry_t *db_entry = dtc_viewer_get_database_entry(viewer, code);
        if (db_entry)
        {
            new_dtc.description = db_entry->description;
            new_dtc.cause = db_entry->cause;
            new_dtc.solution = db_entry->solution;
            new_dtc.severity = db_entry->severity;
        }

        // Add to appropriate list
        if (is_active && viewer->active_count < 32)
        {
            viewer->active_dtcs[viewer->active_count] = new_dtc;
            viewer->active_count++;
        }
        else if (is_pending && viewer->pending_count < 32)
        {
            viewer->pending_dtcs[viewer->pending_count] = new_dtc;
            viewer->pending_count++;
        }
        else if (is_permanent && viewer->permanent_count < 32)
        {
            viewer->permanent_dtcs[viewer->permanent_count] = new_dtc;
            viewer->permanent_count++;
        }
    }

    // Add to history
    if (viewer->history_count < 1000)
    {
        viewer->history[viewer->history_count].code = code;
        viewer->history[viewer->history_count].timestamp = current_time;
        viewer->history[viewer->history_count].was_active = is_active;
        viewer->history[viewer->history_count].was_pending = is_pending;
        viewer->history[viewer->history_count].was_cleared = false;
        viewer->history_count++;
    }

    // Update statistics
    dtc_viewer_update_statistics(viewer);

    return 0;
}

int dtc_viewer_remove_dtc(dtc_viewer_t *viewer, uint16_t code)
{
    if (!viewer || !viewer->is_initialized)
        return -1;

    // Remove from active DTCs
    for (size_t i = 0; i < viewer->active_count; i++)
    {
        if (viewer->active_dtcs[i].code == code)
        {
            // Shift remaining DTCs
            for (size_t j = i; j < viewer->active_count - 1; j++)
            {
                viewer->active_dtcs[j] = viewer->active_dtcs[j + 1];
            }
            viewer->active_count--;
            break;
        }
    }

    // Remove from pending DTCs
    for (size_t i = 0; i < viewer->pending_count; i++)
    {
        if (viewer->pending_dtcs[i].code == code)
        {
            for (size_t j = i; j < viewer->pending_count - 1; j++)
            {
                viewer->pending_dtcs[j] = viewer->pending_dtcs[j + 1];
            }
            viewer->pending_count--;
            break;
        }
    }

    // Remove from permanent DTCs
    for (size_t i = 0; i < viewer->permanent_count; i++)
    {
        if (viewer->permanent_dtcs[i].code == code)
        {
            for (size_t j = i; j < viewer->permanent_count - 1; j++)
            {
                viewer->permanent_dtcs[j] = viewer->permanent_dtcs[j + 1];
            }
            viewer->permanent_count--;
            break;
        }
    }

    // Add to history as cleared
    if (viewer->history_count < 1000)
    {
        viewer->history[viewer->history_count].code = code;
        viewer->history[viewer->history_count].timestamp = time(NULL);
        viewer->history[viewer->history_count].was_active = false;
        viewer->history[viewer->history_count].was_pending = false;
        viewer->history[viewer->history_count].was_cleared = true;
        viewer->history_count++;
    }

    // Update statistics
    dtc_viewer_update_statistics(viewer);

    return 0;
}

int dtc_viewer_clear_all_dtcs(dtc_viewer_t *viewer)
{
    if (!viewer || !viewer->is_initialized)
        return -1;

    viewer->active_count = 0;
    viewer->pending_count = 0;
    viewer->permanent_count = 0;

    // Update statistics
    dtc_viewer_update_statistics(viewer);

    return 0;
}

const dtc_code_t *dtc_viewer_get_dtc(dtc_viewer_t *viewer, uint16_t code)
{
    if (!viewer || !viewer->is_initialized)
        return NULL;

    // Check active DTCs
    for (size_t i = 0; i < viewer->active_count; i++)
    {
        if (viewer->active_dtcs[i].code == code)
        {
            return &viewer->active_dtcs[i];
        }
    }

    // Check pending DTCs
    for (size_t i = 0; i < viewer->pending_count; i++)
    {
        if (viewer->pending_dtcs[i].code == code)
        {
            return &viewer->pending_dtcs[i];
        }
    }

    // Check permanent DTCs
    for (size_t i = 0; i < viewer->permanent_count; i++)
    {
        if (viewer->permanent_dtcs[i].code == code)
        {
            return &viewer->permanent_dtcs[i];
        }
    }

    return NULL;
}

const dtc_database_entry_t *dtc_viewer_get_database_entry(dtc_viewer_t *viewer, uint16_t code)
{
    if (!viewer || !viewer->is_initialized)
        return NULL;

    for (size_t i = 0; i < viewer->database_size; i++)
    {
        if (viewer->database[i].code == code)
        {
            return &viewer->database[i];
        }
    }

    return NULL;
}

const char *dtc_viewer_get_dtc_description(dtc_viewer_t *viewer, uint16_t code)
{
    const dtc_database_entry_t *entry = dtc_viewer_get_database_entry(viewer, code);
    return entry ? entry->description : "Unknown DTC";
}

const char *dtc_viewer_get_dtc_cause(dtc_viewer_t *viewer, uint16_t code)
{
    const dtc_database_entry_t *entry = dtc_viewer_get_database_entry(viewer, code);
    return entry ? entry->cause : "Unknown cause";
}

const char *dtc_viewer_get_dtc_solution(dtc_viewer_t *viewer, uint16_t code)
{
    const dtc_database_entry_t *entry = dtc_viewer_get_database_entry(viewer, code);
    return entry ? entry->solution : "Unknown solution";
}

dtc_severity_t dtc_viewer_get_dtc_severity(dtc_viewer_t *viewer, uint16_t code)
{
    const dtc_database_entry_t *entry = dtc_viewer_get_database_entry(viewer, code);
    return entry ? entry->severity : DTC_SEVERITY_WARNING;
}

int dtc_viewer_update_statistics(dtc_viewer_t *viewer)
{
    if (!viewer || !viewer->is_initialized)
        return -1;

    viewer->statistics.active_dtcs = viewer->active_count;
    viewer->statistics.pending_dtcs = viewer->pending_count;
    viewer->statistics.permanent_dtcs = viewer->permanent_count;
    viewer->statistics.total_dtcs = viewer->active_count + viewer->pending_count + viewer->permanent_count;

    // Calculate total occurrences
    viewer->statistics.total_occurrences = 0;
    for (size_t i = 0; i < viewer->active_count; i++)
    {
        viewer->statistics.total_occurrences += viewer->active_dtcs[i].occurrence_count;
    }
    for (size_t i = 0; i < viewer->pending_count; i++)
    {
        viewer->statistics.total_occurrences += viewer->pending_dtcs[i].occurrence_count;
    }
    for (size_t i = 0; i < viewer->permanent_count; i++)
    {
        viewer->statistics.total_occurrences += viewer->permanent_dtcs[i].occurrence_count;
    }

    return 0;
}

const dtc_statistics_t *dtc_viewer_get_statistics(dtc_viewer_t *viewer)
{
    return viewer ? &viewer->statistics : NULL;
}

// Utility Functions
uint16_t dtc_parse_code(const char *code_string)
{
    if (!code_string)
        return 0;

    // Parse DTC code string (e.g., "P0301" -> 0x0301)
    if (strlen(code_string) >= 5)
    {
        char prefix = code_string[0];
        int category = 0;

        switch (prefix)
        {
        case 'P':
            category = 0;
            break;
        case 'C':
            category = 1;
            break;
        case 'B':
            category = 2;
            break;
        case 'U':
            category = 3;
            break;
        default:
            return 0;
        }

        int code_num = 0;
        if (sscanf(&code_string[1], "%04X", (unsigned int*)&code_num) == 1)
        {
            return (category << 12) | code_num;
        }
    }

    return 0;
}

const char *dtc_code_to_string(uint16_t code)
{
    static char code_string[8];
    char prefix = 'P';

    int category = (code >> 12) & 0xF;
    switch (category)
    {
    case 0:
        prefix = 'P';
        break;
    case 1:
        prefix = 'C';
        break;
    case 2:
        prefix = 'B';
        break;
    case 3:
        prefix = 'U';
        break;
    default:
        prefix = 'P';
        break;
    }

    snprintf(code_string, sizeof(code_string), "%c%04X", prefix, code & 0xFFF);
    return code_string;
}

dtc_category_t dtc_get_category(uint16_t code)
{
    int category = (code >> 12) & 0xF;
    switch (category)
    {
    case 0:
        return DTC_CATEGORY_POWERTRAIN;
    case 1:
        return DTC_CATEGORY_CHASSIS;
    case 2:
        return DTC_CATEGORY_BODY;
    case 3:
        return DTC_CATEGORY_NETWORK;
    default:
        return DTC_CATEGORY_UNKNOWN;
    }
}

const char *dtc_category_to_string(dtc_category_t category)
{
    switch (category)
    {
    case DTC_CATEGORY_POWERTRAIN:
        return "Powertrain";
    case DTC_CATEGORY_CHASSIS:
        return "Chassis";
    case DTC_CATEGORY_BODY:
        return "Body";
    case DTC_CATEGORY_NETWORK:
        return "Network";
    default:
        return "Unknown";
    }
}

const char *dtc_severity_to_string(dtc_severity_t severity)
{
    switch (severity)
    {
    case DTC_SEVERITY_INFO:
        return "Info";
    case DTC_SEVERITY_WARNING:
        return "Warning";
    case DTC_SEVERITY_ERROR:
        return "Error";
    case DTC_SEVERITY_CRITICAL:
        return "Critical";
    case DTC_SEVERITY_EMERGENCY:
        return "Emergency";
    default:
        return "Unknown";
    }
}

bool dtc_is_powertrain(uint16_t code)
{
    return dtc_get_category(code) == DTC_CATEGORY_POWERTRAIN;
}

bool dtc_is_chassis(uint16_t code)
{
    return dtc_get_category(code) == DTC_CATEGORY_CHASSIS;
}

bool dtc_is_body(uint16_t code)
{
    return dtc_get_category(code) == DTC_CATEGORY_BODY;
}

bool dtc_is_network(uint16_t code)
{
    return dtc_get_category(code) == DTC_CATEGORY_NETWORK;
}
