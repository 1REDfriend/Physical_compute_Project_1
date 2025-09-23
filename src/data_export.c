#include "data_export.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

// Export Manager Implementation
export_manager_t *export_manager_create(void)
{
    export_manager_t *manager = (export_manager_t *)calloc(1, sizeof(export_manager_t));
    if (!manager)
        return NULL;

    manager->is_exporting = false;
    manager->is_initialized = false;
    manager->current_file = NULL;
    manager->export_start_time = 0;

    return manager;
}

void export_manager_destroy(export_manager_t *manager)
{
    if (!manager)
        return;

    if (manager->is_exporting)
    {
        export_manager_stop_export(manager);
    }

    if (manager->current_file)
    {
        fclose(manager->current_file);
    }

    free(manager);
}

int export_manager_init(export_manager_t *manager, const export_config_t *config)
{
    if (!manager)
        return -1;

    if (config)
    {
        manager->config = *config;
    }
    else
    {
        export_manager_set_default_config(manager);
    }

    // Validate configuration
    if (!export_manager_validate_config(&manager->config))
    {
        return -1;
    }

    // Create output directory if it doesn't exist
    if (strlen(manager->config.output_directory) > 0)
    {
        mkdir(manager->config.output_directory, 0755);
    }

    manager->is_initialized = true;
    return 0;
}

int export_manager_set_config(export_manager_t *manager, const export_config_t *config)
{
    if (!manager || !config)
        return -1;

    if (!export_manager_validate_config(config))
        return -1;

    manager->config = *config;
    return 0;
}

int export_manager_start_export(export_manager_t *manager)
{
    if (!manager || !manager->is_initialized)
        return -1;

    if (manager->is_exporting)
        return 0; // Already exporting

    manager->is_exporting = true;
    manager->export_start_time = time(NULL);

    // Reset statistics
    memset(&manager->statistics, 0, sizeof(export_statistics_t));

    // Create output file
    if (export_manager_create_output_file(manager) != 0)
    {
        manager->is_exporting = false;
        return -1;
    }

    return 0;
}

int export_manager_stop_export(export_manager_t *manager)
{
    if (!manager || !manager->is_exporting)
        return 0;

    // Finalize export
    export_manager_finalize_export(manager);

    // Close file
    export_manager_close_output_file(manager);

    manager->is_exporting = false;
    manager->statistics.export_time_ms = (time(NULL) - manager->export_start_time) * 1000;
    manager->statistics.success = true;

    return 0;
}

int export_manager_export_data(export_manager_t *manager, const void *data, size_t data_size)
{
    if (!manager || !data || data_size == 0)
        return -1;

    if (!manager->is_exporting)
        return -1;

    int result = -1;
    switch (manager->config.format)
    {
    case EXPORT_FORMAT_CSV:
        result = export_manager_export_csv(manager, data, data_size);
        break;
    case EXPORT_FORMAT_JSON:
        result = export_manager_export_json(manager, data, data_size);
        break;
    case EXPORT_FORMAT_XML:
        result = export_manager_export_xml(manager, data, data_size);
        break;
    case EXPORT_FORMAT_EXCEL:
        result = export_manager_export_excel(manager, data, data_size, NULL);
        break;
    case EXPORT_FORMAT_PDF:
        result = export_manager_export_pdf(manager, data, data_size, NULL);
        break;
    case EXPORT_FORMAT_HTML:
        result = export_manager_export_html(manager, data, data_size);
        break;
    }

    if (result == 0)
    {
        manager->statistics.total_bytes += data_size;
        manager->statistics.total_records++;
    }

    return result;
}

int export_manager_export_obd_data(export_manager_t *manager, const obd_parsed_data_t *data, size_t count)
{
    if (!manager || !data || count == 0)
        return -1;

    if (!manager->is_exporting)
        return -1;

    int result = -1;
    switch (manager->config.format)
    {
    case EXPORT_FORMAT_CSV:
        result = export_manager_convert_obd_to_csv(data, count, NULL, 0);
        break;
    case EXPORT_FORMAT_JSON:
        result = export_manager_convert_obd_to_json(data, count, NULL, 0);
        break;
    case EXPORT_FORMAT_XML:
        result = export_manager_convert_obd_to_xml(data, count, NULL, 0);
        break;
    default:
        return -1;
    }

    if (result == 0)
    {
        manager->statistics.obd_records += count;
    }

    return result;
}

int export_manager_export_dtc_codes(export_manager_t *manager, const dtc_code_t *dtcs, size_t count)
{
    if (!manager || !dtcs || count == 0)
        return -1;

    if (!manager->is_exporting)
        return -1;

    int result = -1;
    switch (manager->config.format)
    {
    case EXPORT_FORMAT_CSV:
        result = export_manager_convert_dtc_to_csv(dtcs, count, NULL, 0);
        break;
    case EXPORT_FORMAT_JSON:
        result = export_manager_convert_dtc_to_json(dtcs, count, NULL, 0);
        break;
    default:
        return -1;
    }

    if (result == 0)
    {
        manager->statistics.dtc_records += count;
    }

    return result;
}

int export_manager_export_log_entries(export_manager_t *manager, const log_entry_t *entries, size_t count)
{
    if (!manager || !entries || count == 0)
        return -1;

    if (!manager->is_exporting)
        return -1;

    int result = -1;
    switch (manager->config.format)
    {
    case EXPORT_FORMAT_CSV:
        result = export_manager_convert_log_to_csv(entries, count, NULL, 0);
        break;
    case EXPORT_FORMAT_JSON:
        result = export_manager_convert_log_to_json(entries, count, NULL, 0);
        break;
    default:
        return -1;
    }

    if (result == 0)
    {
        manager->statistics.log_records += count;
    }

    return result;
}

int export_manager_export_dashboard_data(export_manager_t *manager, const dashboard_state_t *dashboard)
{
    if (!manager || !dashboard)
        return -1;

    if (!manager->is_exporting)
        return -1;

    // Export dashboard data based on format
    int result = -1;
    switch (manager->config.format)
    {
    case EXPORT_FORMAT_CSV:
        // Export gauges as CSV
        result = export_manager_export_csv(manager, dashboard, sizeof(dashboard_state_t));
        break;
    case EXPORT_FORMAT_JSON:
        result = export_manager_export_json(manager, dashboard, sizeof(dashboard_state_t));
        break;
    case EXPORT_FORMAT_HTML:
        result = export_manager_export_html(manager, dashboard, sizeof(dashboard_state_t));
        break;
    default:
        return -1;
    }

    if (result == 0)
    {
        manager->statistics.dashboard_records++;
    }

    return result;
}

int export_manager_export_csv(export_manager_t *manager, const void *data, size_t data_size)
{
    if (!manager || !data || data_size == 0)
        return -1;

    if (!manager->current_file)
        return -1;

    // Write CSV header
    fprintf(manager->current_file, "Timestamp,Type,Source,Message,Value,Unit,DTC_Code,Valid\n");

    // Write data (simplified - would need proper CSV formatting based on data type)
    fprintf(manager->current_file, "%llu,0,Export,Data Export,0.0,,0,true\n",
            (unsigned long long)time(NULL));

    return 0;
}

int export_manager_export_json(export_manager_t *manager, const void *data, size_t data_size)
{
    if (!manager || !data || data_size == 0)
        return -1;

    if (!manager->current_file)
        return -1;

    // Write JSON header
    fprintf(manager->current_file, "{\n");
    fprintf(manager->current_file, "  \"export_info\": {\n");
    fprintf(manager->current_file, "    \"title\": \"%s\",\n", manager->config.title);
    fprintf(manager->current_file, "    \"author\": \"%s\",\n", manager->config.author);
    fprintf(manager->current_file, "    \"description\": \"%s\",\n", manager->config.description);
    fprintf(manager->current_file, "    \"export_time\": %llu,\n", (unsigned long long)time(NULL));
    fprintf(manager->current_file, "    \"format\": \"%s\"\n", export_format_to_string(manager->config.format));
    fprintf(manager->current_file, "  },\n");
    fprintf(manager->current_file, "  \"data\": [\n");

    // Write data (simplified)
    fprintf(manager->current_file, "    {\n");
    fprintf(manager->current_file, "      \"timestamp\": %llu,\n", (unsigned long long)time(NULL));
    fprintf(manager->current_file, "      \"type\": \"export\",\n");
    fprintf(manager->current_file, "      \"message\": \"Data export\"\n");
    fprintf(manager->current_file, "    }\n");

    fprintf(manager->current_file, "  ]\n");
    fprintf(manager->current_file, "}\n");

    return 0;
}

int export_manager_export_xml(export_manager_t *manager, const void *data, size_t data_size)
{
    if (!manager || !data || data_size == 0)
        return -1;

    if (!manager->current_file)
        return -1;

    // Write XML header
    fprintf(manager->current_file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(manager->current_file, "<obd_export>\n");
    fprintf(manager->current_file, "  <export_info>\n");
    fprintf(manager->current_file, "    <title>%s</title>\n", manager->config.title);
    fprintf(manager->current_file, "    <author>%s</author>\n", manager->config.author);
    fprintf(manager->current_file, "    <description>%s</description>\n", manager->config.description);
    fprintf(manager->current_file, "    <export_time>%llu</export_time>\n", (unsigned long long)time(NULL));
    fprintf(manager->current_file, "    <format>%s</format>\n", export_format_to_string(manager->config.format));
    fprintf(manager->current_file, "  </export_info>\n");
    fprintf(manager->current_file, "  <data>\n");

    // Write data (simplified)
    fprintf(manager->current_file, "    <record>\n");
    fprintf(manager->current_file, "      <timestamp>%llu</timestamp>\n", (unsigned long long)time(NULL));
    fprintf(manager->current_file, "      <type>export</type>\n");
    fprintf(manager->current_file, "      <message>Data export</message>\n");
    fprintf(manager->current_file, "    </record>\n");

    fprintf(manager->current_file, "  </data>\n");
    fprintf(manager->current_file, "</obd_export>\n");

    return 0;
}

int export_manager_export_excel(export_manager_t *manager, const void *data, size_t data_size, const excel_export_options_t *options)
{
    (void)options; // Suppress unused parameter warning
    if (!manager || !data || data_size == 0)
        return -1;

    // Excel export would require a library like libxlsxwriter
    // For now, create a CSV file that can be opened in Excel
    return export_manager_export_csv(manager, data, data_size);
}

int export_manager_export_pdf(export_manager_t *manager, const void *data, size_t data_size, const pdf_export_options_t *options)
{
    (void)options; // Suppress unused parameter warning
    if (!manager || !data || data_size == 0)
        return -1;

    // PDF export would require a library like libharu or similar
    // For now, create an HTML file that can be converted to PDF
    return export_manager_export_html(manager, data, data_size);
}

int export_manager_export_html(export_manager_t *manager, const void *data, size_t data_size)
{
    if (!manager || !data || data_size == 0)
        return -1;

    if (!manager->current_file)
        return -1;

    // Write HTML header
    fprintf(manager->current_file, "<!DOCTYPE html>\n");
    fprintf(manager->current_file, "<html>\n");
    fprintf(manager->current_file, "<head>\n");
    fprintf(manager->current_file, "  <title>%s</title>\n", manager->config.title);
    fprintf(manager->current_file, "  <meta charset=\"UTF-8\">\n");
    fprintf(manager->current_file, "  <meta name=\"author\" content=\"%s\">\n", manager->config.author);
    fprintf(manager->current_file, "  <meta name=\"description\" content=\"%s\">\n", manager->config.description);
    fprintf(manager->current_file, "  <style>\n");
    fprintf(manager->current_file, "    body { font-family: Arial, sans-serif; margin: 20px; }\n");
    fprintf(manager->current_file, "    table { border-collapse: collapse; width: 100%%; }\n");
    fprintf(manager->current_file, "    th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n");
    fprintf(manager->current_file, "    th { background-color: #f2f2f2; }\n");
    fprintf(manager->current_file, "  </style>\n");
    fprintf(manager->current_file, "</head>\n");
    fprintf(manager->current_file, "<body>\n");
    fprintf(manager->current_file, "  <h1>%s</h1>\n", manager->config.title);
    fprintf(manager->current_file, "  <p><strong>Author:</strong> %s</p>\n", manager->config.author);
    fprintf(manager->current_file, "  <p><strong>Description:</strong> %s</p>\n", manager->config.description);
    time_t export_time = (time_t)manager->export_start_time;
    fprintf(manager->current_file, "  <p><strong>Export Time:</strong> %s</p>\n", ctime(&export_time));
    fprintf(manager->current_file, "  <p><strong>Format:</strong> %s</p>\n", export_format_to_string(manager->config.format));

    // Write data table
    fprintf(manager->current_file, "  <table>\n");
    fprintf(manager->current_file, "    <tr>\n");
    fprintf(manager->current_file, "      <th>Timestamp</th>\n");
    fprintf(manager->current_file, "      <th>Type</th>\n");
    fprintf(manager->current_file, "      <th>Message</th>\n");
    fprintf(manager->current_file, "    </tr>\n");
    fprintf(manager->current_file, "    <tr>\n");
    fprintf(manager->current_file, "      <td>%llu</td>\n", (unsigned long long)time(NULL));
    fprintf(manager->current_file, "      <td>export</td>\n");
    fprintf(manager->current_file, "      <td>Data export</td>\n");
    fprintf(manager->current_file, "    </tr>\n");
    fprintf(manager->current_file, "  </table>\n");

    fprintf(manager->current_file, "</body>\n");
    fprintf(manager->current_file, "</html>\n");

    return 0;
}

int export_manager_create_output_file(export_manager_t *manager)
{
    if (!manager)
        return -1;

    // Generate filename with timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char filename[512];
    snprintf(filename, sizeof(filename), "%s/%s_%04d%02d%02d_%02d%02d%02d.%s",
             manager->config.output_directory,
             manager->config.output_filename,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             (manager->config.format == EXPORT_FORMAT_CSV) ? "csv" : (manager->config.format == EXPORT_FORMAT_JSON) ? "json"
                                                                 : (manager->config.format == EXPORT_FORMAT_XML)    ? "xml"
                                                                 : (manager->config.format == EXPORT_FORMAT_EXCEL)  ? "xlsx"
                                                                 : (manager->config.format == EXPORT_FORMAT_PDF)    ? "pdf"
                                                                                                                    : "html");

    strncpy(manager->current_filename, filename, sizeof(manager->current_filename) - 1);

    // Open file
    manager->current_file = fopen(filename, "w");
    if (!manager->current_file)
        return -1;

    return 0;
}

int export_manager_close_output_file(export_manager_t *manager)
{
    if (!manager || !manager->current_file)
        return -1;

    fclose(manager->current_file);
    manager->current_file = NULL;

    return 0;
}

int export_manager_finalize_export(export_manager_t *manager)
{
    if (!manager || !manager->is_exporting)
        return -1;

    // Add footer based on format
    if (manager->current_file)
    {
        switch (manager->config.format)
        {
        case EXPORT_FORMAT_JSON:
            // JSON footer already handled in export functions
            break;
        case EXPORT_FORMAT_XML:
            // XML footer already handled in export functions
            break;
        case EXPORT_FORMAT_HTML:
            // HTML footer already handled in export functions
            break;
        default:
            break;
        }
    }

    return 0;
}

// Utility functions
const char *export_format_to_string(export_format_t format)
{
    switch (format)
    {
    case EXPORT_FORMAT_CSV:
        return "CSV";
    case EXPORT_FORMAT_JSON:
        return "JSON";
    case EXPORT_FORMAT_XML:
        return "XML";
    case EXPORT_FORMAT_EXCEL:
        return "Excel";
    case EXPORT_FORMAT_PDF:
        return "PDF";
    case EXPORT_FORMAT_HTML:
        return "HTML";
    default:
        return "Unknown";
    }
}

const char *export_data_type_to_string(export_data_type_t data_type)
{
    switch (data_type)
    {
    case EXPORT_DATA_OBD_LIVE:
        return "OBD Live Data";
    case EXPORT_DATA_DTC_CODES:
        return "DTC Codes";
    case EXPORT_DATA_LOG_ENTRIES:
        return "Log Entries";
    case EXPORT_DATA_DASHBOARD:
        return "Dashboard Data";
    case EXPORT_DATA_STATISTICS:
        return "Statistics";
    case EXPORT_DATA_ALL:
        return "All Data";
    default:
        return "Unknown";
    }
}

int export_manager_set_default_config(export_manager_t *manager)
{
    if (!manager)
        return -1;

    manager->config.format = EXPORT_FORMAT_CSV;
    manager->config.data_type = EXPORT_DATA_ALL;
    strcpy(manager->config.output_filename, "obd_export");
    strcpy(manager->config.output_directory, "./exports");
    manager->config.include_timestamps = true;
    manager->config.include_metadata = true;
    manager->config.compress_output = false;
    manager->config.include_charts = false;
    manager->config.include_statistics = true;
    manager->config.start_time = 0;
    manager->config.end_time = 0;
    strcpy(manager->config.title, "OBD-II Data Export");
    strcpy(manager->config.author, "OBD-II Reader");
    strcpy(manager->config.description, "Exported OBD-II data");

    return 0;
}

int export_manager_set_csv_config(export_manager_t *manager)
{
    if (!manager)
        return -1;

    export_manager_set_default_config(manager);
    manager->config.format = EXPORT_FORMAT_CSV;
    manager->config.include_charts = false;
    manager->config.include_statistics = true;

    return 0;
}

int export_manager_set_json_config(export_manager_t *manager)
{
    if (!manager)
        return -1;

    export_manager_set_default_config(manager);
    manager->config.format = EXPORT_FORMAT_JSON;
    manager->config.include_charts = true;
    manager->config.include_statistics = true;

    return 0;
}

int export_manager_set_excel_config(export_manager_t *manager)
{
    if (!manager)
        return -1;

    export_manager_set_default_config(manager);
    manager->config.format = EXPORT_FORMAT_EXCEL;
    manager->config.include_charts = true;
    manager->config.include_statistics = true;

    return 0;
}

int export_manager_set_pdf_config(export_manager_t *manager)
{
    if (!manager)
        return -1;

    export_manager_set_default_config(manager);
    manager->config.format = EXPORT_FORMAT_PDF;
    manager->config.include_charts = true;
    manager->config.include_statistics = true;

    return 0;
}

int export_manager_set_html_config(export_manager_t *manager)
{
    if (!manager)
        return -1;

    export_manager_set_default_config(manager);
    manager->config.format = EXPORT_FORMAT_HTML;
    manager->config.include_charts = true;
    manager->config.include_statistics = true;

    return 0;
}

// Data conversion functions (simplified implementations)
int export_manager_convert_obd_to_csv(const obd_parsed_data_t *obd_data, size_t count, char *csv_data, size_t csv_size)
{
    if (!obd_data || count == 0)
        return -1;

    // Simplified CSV conversion
    if (csv_data && csv_size > 0)
    {
        strncpy(csv_data, "Timestamp,Mode,PID,Payload,Status\n", csv_size - 1);
    }

    return 0;
}

int export_manager_convert_obd_to_json(const obd_parsed_data_t *obd_data, size_t count, char *json_data, size_t json_size)
{
    if (!obd_data || count == 0)
        return -1;

    // Simplified JSON conversion
    if (json_data && json_size > 0)
    {
        strncpy(json_data, "{\"obd_data\": []}", json_size - 1);
    }

    return 0;
}

int export_manager_convert_obd_to_xml(const obd_parsed_data_t *obd_data, size_t count, char *xml_data, size_t xml_size)
{
    if (!obd_data || count == 0)
        return -1;

    // Simplified XML conversion
    if (xml_data && xml_size > 0)
    {
        strncpy(xml_data, "<obd_data></obd_data>", xml_size - 1);
    }

    return 0;
}

int export_manager_convert_dtc_to_csv(const dtc_code_t *dtc_data, size_t count, char *csv_data, size_t csv_size)
{
    if (!dtc_data || count == 0)
        return -1;

    // Simplified CSV conversion
    if (csv_data && csv_size > 0)
    {
        strncpy(csv_data, "Code,Description,Cause,Solution\n", csv_size - 1);
    }

    return 0;
}

int export_manager_convert_dtc_to_json(const dtc_code_t *dtc_data, size_t count, char *json_data, size_t json_size)
{
    if (!dtc_data || count == 0)
        return -1;

    // Simplified JSON conversion
    if (json_data && json_size > 0)
    {
        strncpy(json_data, "{\"dtc_codes\": []}", json_size - 1);
    }

    return 0;
}

int export_manager_convert_log_to_csv(const log_entry_t *log_data, size_t count, char *csv_data, size_t csv_size)
{
    if (!log_data || count == 0)
        return -1;

    // Simplified CSV conversion
    if (csv_data && csv_size > 0)
    {
        strncpy(csv_data, "Timestamp,Type,Source,Message,Value,Unit\n", csv_size - 1);
    }

    return 0;
}

int export_manager_convert_log_to_json(const log_entry_t *log_data, size_t count, char *json_data, size_t json_size)
{
    if (!log_data || count == 0)
        return -1;

    // Simplified JSON conversion
    if (json_data && json_size > 0)
    {
        strncpy(json_data, "{\"log_entries\": []}", json_size - 1);
    }

    return 0;
}

// Validation functions
bool export_manager_validate_config(const export_config_t *config)
{
    if (!config)
        return false;

    // Check required fields
    if (strlen(config->output_filename) == 0)
        return false;
    if (strlen(config->output_directory) == 0)
        return false;

    // Check format validity
    if (config->format < EXPORT_FORMAT_CSV || config->format > EXPORT_FORMAT_HTML)
        return false;

    // Check data type validity
    if (config->data_type < EXPORT_DATA_OBD_LIVE || config->data_type > EXPORT_DATA_ALL)
        return false;

    return true;
}

bool export_manager_validate_data(const void *data, size_t data_size, export_data_type_t data_type)
{
    if (!data || data_size == 0)
        return false;

    // Basic validation based on data type
    switch (data_type)
    {
    case EXPORT_DATA_OBD_LIVE:
        return data_size >= sizeof(obd_parsed_data_t);
    case EXPORT_DATA_DTC_CODES:
        return data_size >= sizeof(dtc_code_t);
    case EXPORT_DATA_LOG_ENTRIES:
        return data_size >= sizeof(log_entry_t);
    case EXPORT_DATA_DASHBOARD:
        return data_size >= sizeof(dashboard_state_t);
    default:
        return true;
    }
}

bool export_manager_validate_filename(const char *filename, export_format_t format)
{
    if (!filename)
        return false;

    const char *expected_extensions[] = {
        "csv", "json", "xml", "xlsx", "pdf", "html"};

    if (format < EXPORT_FORMAT_CSV || format > EXPORT_FORMAT_HTML)
        return false;

    const char *expected_ext = expected_extensions[format];
    size_t filename_len = strlen(filename);
    size_t ext_len = strlen(expected_ext);

    if (filename_len < ext_len + 1)
        return false;

    return strcmp(filename + filename_len - ext_len - 1, ".") == 0 &&
           strcmp(filename + filename_len - ext_len, expected_ext) == 0;
}

// Statistics
const export_statistics_t *export_manager_get_statistics(export_manager_t *manager)
{
    return manager ? &manager->statistics : NULL;
}

int export_manager_reset_statistics(export_manager_t *manager)
{
    if (!manager)
        return -1;

    memset(&manager->statistics, 0, sizeof(export_statistics_t));
    return 0;
}

// Error handling
const char *export_manager_get_last_error(export_manager_t *manager)
{
    return manager ? manager->statistics.error_message : "Invalid manager";
}

int export_manager_set_error(export_manager_t *manager, const char *error_message)
{
    if (!manager || !error_message)
        return -1;

    strncpy(manager->statistics.error_message, error_message, sizeof(manager->statistics.error_message) - 1);
    manager->statistics.success = false;

    return 0;
}

int export_manager_clear_error(export_manager_t *manager)
{
    if (!manager)
        return -1;

    memset(manager->statistics.error_message, 0, sizeof(manager->statistics.error_message));
    manager->statistics.success = true;

    return 0;
}
