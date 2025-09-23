#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log Entry Types
typedef enum {
    LOG_ENTRY_OBD_DATA = 0,
    LOG_ENTRY_DTC_CODE = 1,
    LOG_ENTRY_SYSTEM_EVENT = 2,
    LOG_ENTRY_USER_ACTION = 3,
    LOG_ENTRY_ERROR = 4
} log_entry_type_t;

// Log Entry Structure
typedef struct {
    uint64_t timestamp;
    log_entry_type_t type;
    char source[32];
    char message[256];
    float value;
    const char* unit;
    uint16_t dtc_code;
    bool is_valid;
} log_entry_t;

// Log File Format
typedef enum {
    LOG_FORMAT_CSV = 0,
    LOG_FORMAT_JSON = 1,
    LOG_FORMAT_BINARY = 2
} log_format_t;

// Log Configuration
typedef struct {
    bool auto_log;
    uint32_t log_interval_ms;
    log_format_t format;
    char log_directory[256];
    char log_filename[128];
    size_t max_file_size_mb;
    size_t max_files;
    bool compress_old_files;
    bool include_timestamps;
    bool include_metadata;
} log_config_t;

// Log Statistics
typedef struct {
    uint64_t total_entries;
    uint64_t obd_entries;
    uint64_t dtc_entries;
    uint64_t system_entries;
    uint64_t user_entries;
    uint64_t error_entries;
    uint64_t total_bytes;
    uint64_t files_created;
    uint64_t last_log_time;
    uint64_t first_log_time;
} log_statistics_t;

// Data Logger State
typedef struct {
    log_entry_t* entries;
    size_t entry_count;
    size_t entry_capacity;
    log_config_t config;
    log_statistics_t statistics;
    bool is_logging;
    bool is_initialized;
    char current_filename[256];
    FILE* current_file;
    uint64_t session_start_time;
} data_logger_t;

// Function prototypes
data_logger_t* data_logger_create(void);
void data_logger_destroy(data_logger_t* logger);

// Initialization
int data_logger_init(data_logger_t* logger, const log_config_t* config);
int data_logger_set_config(data_logger_t* logger, const log_config_t* config);

// Logging operations
int data_logger_start_logging(data_logger_t* logger);
int data_logger_stop_logging(data_logger_t* logger);
int data_logger_pause_logging(data_logger_t* logger);
int data_logger_resume_logging(data_logger_t* logger);

// Entry management
int data_logger_add_entry(data_logger_t* logger, const log_entry_t* entry);
int data_logger_add_obd_data(data_logger_t* logger, const char* source, float value, const char* unit);
int data_logger_add_dtc_code(data_logger_t* logger, uint16_t dtc_code, const char* description);
int data_logger_add_system_event(data_logger_t* logger, const char* event, const char* details);
int data_logger_add_user_action(data_logger_t* logger, const char* action, const char* details);
int data_logger_add_error(data_logger_t* logger, const char* error, const char* details);

// File operations
int data_logger_create_log_file(data_logger_t* logger);
int data_logger_close_log_file(data_logger_t* logger);
int data_logger_flush_log_file(data_logger_t* logger);
int data_logger_rotate_log_file(data_logger_t* logger);

// Data export
int data_logger_export_csv(data_logger_t* logger, const char* filename);
int data_logger_export_json(data_logger_t* logger, const char* filename);
int data_logger_export_binary(data_logger_t* logger, const char* filename);
int data_logger_export_range(data_logger_t* logger, const char* filename, uint64_t start_time, uint64_t end_time);

// Data import
int data_logger_import_csv(data_logger_t* logger, const char* filename);
int data_logger_import_json(data_logger_t* logger, const char* filename);
int data_logger_import_binary(data_logger_t* logger, const char* filename);

// Query operations
const log_entry_t* data_logger_get_entry(data_logger_t* logger, size_t index);
size_t data_logger_get_entry_count(data_logger_t* logger);
int data_logger_get_entries_by_type(data_logger_t* logger, log_entry_type_t type, log_entry_t* entries, size_t max_entries, size_t* count);
int data_logger_get_entries_by_time_range(data_logger_t* logger, uint64_t start_time, uint64_t end_time, log_entry_t* entries, size_t max_entries, size_t* count);
int data_logger_get_entries_by_source(data_logger_t* logger, const char* source, log_entry_t* entries, size_t max_entries, size_t* count);

// Statistics
const log_statistics_t* data_logger_get_statistics(data_logger_t* logger);
int data_logger_update_statistics(data_logger_t* logger);
int data_logger_reset_statistics(data_logger_t* logger);

// Utility functions
const char* log_entry_type_to_string(log_entry_type_t type);
const char* log_format_to_string(log_format_t format);
uint64_t data_logger_get_current_timestamp(void);
int data_logger_set_log_directory(data_logger_t* logger, const char* directory);
int data_logger_set_log_filename(data_logger_t* logger, const char* filename);

// Pre-defined configurations
int data_logger_set_default_config(data_logger_t* logger);
int data_logger_set_high_frequency_config(data_logger_t* logger);
int data_logger_set_compact_config(data_logger_t* logger);
int data_logger_set_debug_config(data_logger_t* logger);

// File management
int data_logger_list_log_files(data_logger_t* logger, char** filenames, size_t max_files, size_t* file_count);
int data_logger_delete_log_file(data_logger_t* logger, const char* filename);
int data_logger_compress_log_file(data_logger_t* logger, const char* filename);
int data_logger_cleanup_old_files(data_logger_t* logger);

#ifdef __cplusplus
}
#endif

#endif // DATA_LOGGER_H
