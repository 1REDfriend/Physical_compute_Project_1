#ifndef DTC_VIEWER_H
#define DTC_VIEWER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// DTC Code Structure
typedef struct {
    uint16_t code;              // DTC code (e.g., P0301)
    char code_string[8];        // String representation (e.g., "P0301")
    const char* description;    // Human-readable description
    const char* cause;          // Possible causes
    const char* solution;       // Recommended solution
    uint8_t severity;           // Severity level (1-5, 5 being critical)
    bool is_active;            // Currently active
    bool is_pending;           // Pending (not yet active)
    bool is_permanent;         // Permanent (cannot be cleared)
    uint64_t first_seen;       // First occurrence timestamp
    uint64_t last_seen;        // Last occurrence timestamp
    uint32_t occurrence_count; // Number of times this DTC has occurred
} dtc_code_t;

// DTC Category
typedef enum {
    DTC_CATEGORY_POWERTRAIN = 0,    // P0xxx - P3xxx
    DTC_CATEGORY_CHASSIS = 1,       // C0xxx - C3xxx
    DTC_CATEGORY_BODY = 2,          // B0xxx - B3xxx
    DTC_CATEGORY_NETWORK = 3,       // U0xxx - U3xxx
    DTC_CATEGORY_UNKNOWN = 4
} dtc_category_t;

// DTC Severity Levels
typedef enum {
    DTC_SEVERITY_INFO = 1,      // Informational
    DTC_SEVERITY_WARNING = 2,   // Warning
    DTC_SEVERITY_ERROR = 3,     // Error
    DTC_SEVERITY_CRITICAL = 4,  // Critical
    DTC_SEVERITY_EMERGENCY = 5  // Emergency
} dtc_severity_t;

// DTC Database Entry
typedef struct {
    uint16_t code;
    char code_string[8];
    const char* description;
    const char* cause;
    const char* solution;
    dtc_category_t category;
    dtc_severity_t severity;
    const char* affected_systems[8]; // Systems affected by this DTC
    size_t affected_system_count;
} dtc_database_entry_t;

// DTC History Entry
typedef struct {
    uint16_t code;
    uint64_t timestamp;
    bool was_active;
    bool was_pending;
    bool was_cleared;
} dtc_history_entry_t;

// DTC Statistics
typedef struct {
    uint32_t total_dtcs;
    uint32_t active_dtcs;
    uint32_t pending_dtcs;
    uint32_t permanent_dtcs;
    uint32_t cleared_dtcs;
    uint64_t total_occurrences;
    uint64_t first_dtc_time;
    uint64_t last_dtc_time;
} dtc_statistics_t;

// DTC Viewer State
typedef struct {
    dtc_code_t* active_dtcs;
    dtc_code_t* pending_dtcs;
    dtc_code_t* permanent_dtcs;
    dtc_history_entry_t* history;
    dtc_database_entry_t* database;
    dtc_statistics_t statistics;
    size_t active_count;
    size_t pending_count;
    size_t permanent_count;
    size_t history_count;
    size_t database_size;
    bool is_initialized;
} dtc_viewer_t;

// Function prototypes
dtc_viewer_t* dtc_viewer_create(void);
void dtc_viewer_destroy(dtc_viewer_t* viewer);

// Initialization
int dtc_viewer_init(dtc_viewer_t* viewer);
int dtc_viewer_load_database(dtc_viewer_t* viewer, const char* database_file);

// DTC Management
int dtc_viewer_add_dtc(dtc_viewer_t* viewer, uint16_t code, bool is_active, bool is_pending, bool is_permanent);
int dtc_viewer_remove_dtc(dtc_viewer_t* viewer, uint16_t code);
int dtc_viewer_clear_all_dtcs(dtc_viewer_t* viewer);
int dtc_viewer_clear_pending_dtcs(dtc_viewer_t* viewer);
int dtc_viewer_clear_permanent_dtcs(dtc_viewer_t* viewer);

// DTC Queries
const dtc_code_t* dtc_viewer_get_dtc(dtc_viewer_t* viewer, uint16_t code);
const dtc_code_t* dtc_viewer_get_active_dtc(dtc_viewer_t* viewer, size_t index);
const dtc_code_t* dtc_viewer_get_pending_dtc(dtc_viewer_t* viewer, size_t index);
const dtc_code_t* dtc_viewer_get_permanent_dtc(dtc_viewer_t* viewer, size_t index);
size_t dtc_viewer_get_active_dtc_count(dtc_viewer_t* viewer);
size_t dtc_viewer_get_pending_dtc_count(dtc_viewer_t* viewer);
size_t dtc_viewer_get_permanent_dtc_count(dtc_viewer_t* viewer);

// DTC Information
const dtc_database_entry_t* dtc_viewer_get_database_entry(dtc_viewer_t* viewer, uint16_t code);
const char* dtc_viewer_get_dtc_description(dtc_viewer_t* viewer, uint16_t code);
const char* dtc_viewer_get_dtc_cause(dtc_viewer_t* viewer, uint16_t code);
const char* dtc_viewer_get_dtc_solution(dtc_viewer_t* viewer, uint16_t code);
dtc_severity_t dtc_viewer_get_dtc_severity(dtc_viewer_t* viewer, uint16_t code);

// History Management
int dtc_viewer_add_history_entry(dtc_viewer_t* viewer, uint16_t code, bool was_active, bool was_pending, bool was_cleared);
const dtc_history_entry_t* dtc_viewer_get_history_entry(dtc_viewer_t* viewer, size_t index);
size_t dtc_viewer_get_history_count(dtc_viewer_t* viewer);
int dtc_viewer_clear_history(dtc_viewer_t* viewer);

// Statistics
const dtc_statistics_t* dtc_viewer_get_statistics(dtc_viewer_t* viewer);
int dtc_viewer_update_statistics(dtc_viewer_t* viewer);

// Utility Functions
uint16_t dtc_parse_code(const char* code_string);
const char* dtc_code_to_string(uint16_t code);
dtc_category_t dtc_get_category(uint16_t code);
const char* dtc_category_to_string(dtc_category_t category);
const char* dtc_severity_to_string(dtc_severity_t severity);
bool dtc_is_powertrain(uint16_t code);
bool dtc_is_chassis(uint16_t code);
bool dtc_is_body(uint16_t code);
bool dtc_is_network(uint16_t code);

// Database Management
int dtc_viewer_add_database_entry(dtc_viewer_t* viewer, const dtc_database_entry_t* entry);
int dtc_viewer_remove_database_entry(dtc_viewer_t* viewer, uint16_t code);
int dtc_viewer_update_database_entry(dtc_viewer_t* viewer, const dtc_database_entry_t* entry);

// Export/Import
int dtc_viewer_export_dtcs(dtc_viewer_t* viewer, const char* filename);
int dtc_viewer_import_dtcs(dtc_viewer_t* viewer, const char* filename);
int dtc_viewer_export_history(dtc_viewer_t* viewer, const char* filename);
int dtc_viewer_import_history(dtc_viewer_t* viewer, const char* filename);

#ifdef __cplusplus
}
#endif

#endif // DTC_VIEWER_H
