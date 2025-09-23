#ifndef DATA_EXPORT_H
#define DATA_EXPORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "obd_parser.h"
#include "dashboard.h"
#include "dtc_viewer.h"
#include "data_logger.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Export Formats
typedef enum {
    EXPORT_FORMAT_CSV = 0,
    EXPORT_FORMAT_JSON = 1,
    EXPORT_FORMAT_XML = 2,
    EXPORT_FORMAT_EXCEL = 3,
    EXPORT_FORMAT_PDF = 4,
    EXPORT_FORMAT_HTML = 5
} export_format_t;

// Export Data Types
typedef enum {
    EXPORT_DATA_OBD_LIVE = 0,
    EXPORT_DATA_DTC_CODES = 1,
    EXPORT_DATA_LOG_ENTRIES = 2,
    EXPORT_DATA_DASHBOARD = 3,
    EXPORT_DATA_STATISTICS = 4,
    EXPORT_DATA_ALL = 5
} export_data_type_t;

// Export Configuration
typedef struct {
    export_format_t format;
    export_data_type_t data_type;
    char output_filename[256];
    char output_directory[256];
    bool include_timestamps;
    bool include_metadata;
    bool compress_output;
    bool include_charts;
    bool include_statistics;
    uint64_t start_time;
    uint64_t end_time;
    char title[128];
    char author[64];
    char description[256];
} export_config_t;

// Excel Export Options
typedef struct {
    bool create_worksheets;
    bool include_charts;
    bool include_formulas;
    bool auto_format;
    bool freeze_panes;
    char sheet_names[8][32];
    size_t sheet_count;
} excel_export_options_t;

// PDF Export Options
typedef struct {
    char title[128];
    char author[64];
    char subject[128];
    char keywords[256];
    bool include_toc;
    bool include_charts;
    bool include_statistics;
    int page_orientation; // 0=portrait, 1=landscape
    float margin_top;
    float margin_bottom;
    float margin_left;
    float margin_right;
} pdf_export_options_t;

// Export Statistics
typedef struct {
    uint64_t total_records;
    uint64_t obd_records;
    uint64_t dtc_records;
    uint64_t log_records;
    uint64_t dashboard_records;
    uint64_t total_bytes;
    uint64_t export_time_ms;
    bool success;
    char error_message[256];
} export_statistics_t;

// Export Manager
typedef struct {
    export_config_t config;
    export_statistics_t statistics;
    bool is_exporting;
    bool is_initialized;
    char current_filename[256];
    FILE* current_file;
    uint64_t export_start_time;
} export_manager_t;

// Function prototypes
export_manager_t* export_manager_create(void);
void export_manager_destroy(export_manager_t* manager);

// Initialization
int export_manager_init(export_manager_t* manager, const export_config_t* config);
int export_manager_set_config(export_manager_t* manager, const export_config_t* config);

// Export operations
int export_manager_start_export(export_manager_t* manager);
int export_manager_stop_export(export_manager_t* manager);
int export_manager_export_data(export_manager_t* manager, const void* data, size_t data_size);
int export_manager_finalize_export(export_manager_t* manager);

// Specific export functions
int export_manager_export_obd_data(export_manager_t* manager, const obd_parsed_data_t* data, size_t count);
int export_manager_export_dtc_codes(export_manager_t* manager, const dtc_code_t* dtcs, size_t count);
int export_manager_export_log_entries(export_manager_t* manager, const log_entry_t* entries, size_t count);
int export_manager_export_dashboard_data(export_manager_t* manager, const dashboard_state_t* dashboard);
int export_manager_export_statistics(export_manager_t* manager, const void* stats, size_t stats_size);

// Format-specific exports
int export_manager_export_csv(export_manager_t* manager, const void* data, size_t data_size);
int export_manager_export_json(export_manager_t* manager, const void* data, size_t data_size);
int export_manager_export_xml(export_manager_t* manager, const void* data, size_t data_size);
int export_manager_export_excel(export_manager_t* manager, const void* data, size_t data_size, const excel_export_options_t* options);
int export_manager_export_pdf(export_manager_t* manager, const void* data, size_t data_size, const pdf_export_options_t* options);
int export_manager_export_html(export_manager_t* manager, const void* data, size_t data_size);

// Import operations
int export_manager_import_data(export_manager_t* manager, const char* filename, void* data, size_t* data_size);
int export_manager_import_csv(export_manager_t* manager, const char* filename, void* data, size_t* data_size);
int export_manager_import_json(export_manager_t* manager, const char* filename, void* data, size_t* data_size);
int export_manager_import_xml(export_manager_t* manager, const char* filename, void* data, size_t* data_size);

// Utility functions
const char* export_format_to_string(export_format_t format);
const char* export_data_type_to_string(export_data_type_t data_type);
int export_manager_set_output_filename(export_manager_t* manager, const char* filename);
int export_manager_set_output_directory(export_manager_t* manager, const char* directory);
int export_manager_set_time_range(export_manager_t* manager, uint64_t start_time, uint64_t end_time);

// Statistics
const export_statistics_t* export_manager_get_statistics(export_manager_t* manager);
int export_manager_reset_statistics(export_manager_t* manager);

// Pre-defined configurations
int export_manager_set_default_config(export_manager_t* manager);
int export_manager_set_csv_config(export_manager_t* manager);
int export_manager_set_json_config(export_manager_t* manager);
int export_manager_set_excel_config(export_manager_t* manager);
int export_manager_set_pdf_config(export_manager_t* manager);
int export_manager_set_html_config(export_manager_t* manager);

// File operations
int export_manager_create_output_file(export_manager_t* manager);
int export_manager_close_output_file(export_manager_t* manager);
int export_manager_compress_output_file(export_manager_t* manager);
int export_manager_cleanup_temp_files(export_manager_t* manager);

// Data conversion
int export_manager_convert_obd_to_csv(const obd_parsed_data_t* obd_data, size_t count, char* csv_data, size_t csv_size);
int export_manager_convert_obd_to_json(const obd_parsed_data_t* obd_data, size_t count, char* json_data, size_t json_size);
int export_manager_convert_obd_to_xml(const obd_parsed_data_t* obd_data, size_t count, char* xml_data, size_t xml_size);
int export_manager_convert_dtc_to_csv(const dtc_code_t* dtc_data, size_t count, char* csv_data, size_t csv_size);
int export_manager_convert_dtc_to_json(const dtc_code_t* dtc_data, size_t count, char* json_data, size_t json_size);
int export_manager_convert_log_to_csv(const log_entry_t* log_data, size_t count, char* csv_data, size_t csv_size);
int export_manager_convert_log_to_json(const log_entry_t* log_data, size_t count, char* json_data, size_t json_size);

// Validation
bool export_manager_validate_config(const export_config_t* config);
bool export_manager_validate_data(const void* data, size_t data_size, export_data_type_t data_type);
bool export_manager_validate_filename(const char* filename, export_format_t format);

// Error handling
const char* export_manager_get_last_error(export_manager_t* manager);
int export_manager_set_error(export_manager_t* manager, const char* error_message);
int export_manager_clear_error(export_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif // DATA_EXPORT_H
