#include "data_logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

// Data Logger Implementation
data_logger_t *data_logger_create(void)
{
    data_logger_t *logger = (data_logger_t *)calloc(1, sizeof(data_logger_t));
    if (!logger)
        return NULL;

    logger->entries = NULL;
    logger->entry_count = 0;
    logger->entry_capacity = 0;
    logger->is_logging = false;
    logger->is_initialized = false;
    logger->current_file = NULL;
    logger->session_start_time = 0;

    return logger;
}

void data_logger_destroy(data_logger_t *logger)
{
    if (!logger)
        return;

    if (logger->is_logging)
    {
        data_logger_stop_logging(logger);
    }

    if (logger->entries)
    {
        free(logger->entries);
    }

    if (logger->current_file)
    {
        fclose(logger->current_file);
    }

    free(logger);
}

int data_logger_init(data_logger_t *logger, const log_config_t *config)
{
    if (!logger)
        return -1;

    // Set configuration
    if (config)
    {
        logger->config = *config;
    }
    else
    {
        data_logger_set_default_config(logger);
    }

    // Initialize entry buffer
    logger->entry_capacity = 10000; // Start with 10k entries
    logger->entries = (log_entry_t *)calloc(logger->entry_capacity, sizeof(log_entry_t));
    if (!logger->entries)
        return -1;

    // Initialize statistics
    memset(&logger->statistics, 0, sizeof(log_statistics_t));

    // Create log directory if it doesn't exist
    if (strlen(logger->config.log_directory) > 0)
    {
        mkdir(logger->config.log_directory, 0755);
    }

    logger->is_initialized = true;
    return 0;
}

int data_logger_set_config(data_logger_t *logger, const log_config_t *config)
{
    if (!logger || !config)
        return -1;

    logger->config = *config;
    return 0;
}

int data_logger_start_logging(data_logger_t *logger)
{
    if (!logger || !logger->is_initialized)
        return -1;

    if (logger->is_logging)
        return 0; // Already logging

    logger->is_logging = true;
    logger->session_start_time = data_logger_get_current_timestamp();

    // Create log file
    if (data_logger_create_log_file(logger) != 0)
    {
        logger->is_logging = false;
        return -1;
    }

    return 0;
}

int data_logger_stop_logging(data_logger_t *logger)
{
    if (!logger || !logger->is_initialized)
        return -1;

    if (!logger->is_logging)
        return 0; // Already stopped

    // Flush and close current file
    if (logger->current_file)
    {
        data_logger_flush_log_file(logger);
        data_logger_close_log_file(logger);
    }

    logger->is_logging = false;
    return 0;
}

int data_logger_pause_logging(data_logger_t *logger)
{
    if (!logger)
        return -1;

    logger->is_logging = false;
    return 0;
}

int data_logger_resume_logging(data_logger_t *logger)
{
    if (!logger)
        return -1;

    logger->is_logging = true;
    return 0;
}

int data_logger_add_entry(data_logger_t *logger, const log_entry_t *entry)
{
    if (!logger || !entry || !logger->is_initialized)
        return -1;

    // Check if we need to expand the buffer
    if (logger->entry_count >= logger->entry_capacity)
    {
        size_t new_capacity = logger->entry_capacity * 2;
        log_entry_t *new_entries = (log_entry_t *)realloc(logger->entries, new_capacity * sizeof(log_entry_t));
        if (!new_entries)
            return -1;

        logger->entries = new_entries;
        logger->entry_capacity = new_capacity;
    }

    // Add entry
    logger->entries[logger->entry_count] = *entry;
    logger->entry_count++;

    // Update statistics
    logger->statistics.total_entries++;
    switch (entry->type)
    {
    case LOG_ENTRY_OBD_DATA:
        logger->statistics.obd_entries++;
        break;
    case LOG_ENTRY_DTC_CODE:
        logger->statistics.dtc_entries++;
        break;
    case LOG_ENTRY_SYSTEM_EVENT:
        logger->statistics.system_entries++;
        break;
    case LOG_ENTRY_USER_ACTION:
        logger->statistics.user_entries++;
        break;
    case LOG_ENTRY_ERROR:
        logger->statistics.error_entries++;
        break;
    }

    // Write to file if logging
    if (logger->is_logging && logger->current_file)
    {
        // data_logger_write_entry_to_file(logger, entry);
    }

    return 0;
}

int data_logger_add_obd_data(data_logger_t *logger, const char *source, float value, const char *unit)
{
    if (!logger || !source)
        return -1;

    log_entry_t entry = {0};
    entry.timestamp = data_logger_get_current_timestamp();
    entry.type = LOG_ENTRY_OBD_DATA;
    strncpy(entry.source, source, sizeof(entry.source) - 1);
    entry.value = value;
    entry.unit = unit;
    entry.is_valid = true;

    return data_logger_add_entry(logger, &entry);
}

int data_logger_add_dtc_code(data_logger_t *logger, uint16_t dtc_code, const char *description)
{
    if (!logger)
        return -1;

    log_entry_t entry = {0};
    entry.timestamp = data_logger_get_current_timestamp();
    entry.type = LOG_ENTRY_DTC_CODE;
    strncpy(entry.source, "DTC_Scanner", sizeof(entry.source) - 1);
    entry.dtc_code = dtc_code;
    if (description)
    {
        strncpy(entry.message, description, sizeof(entry.message) - 1);
    }
    entry.is_valid = true;

    return data_logger_add_entry(logger, &entry);
}

int data_logger_add_system_event(data_logger_t *logger, const char *event, const char *details)
{
    if (!logger || !event)
        return -1;

    log_entry_t entry = {0};
    entry.timestamp = data_logger_get_current_timestamp();
    entry.type = LOG_ENTRY_SYSTEM_EVENT;
    strncpy(entry.source, "System", sizeof(entry.source) - 1);
    strncpy(entry.message, event, sizeof(entry.message) - 1);
    if (details)
    {
        // Combine event and details
        snprintf(entry.message, sizeof(entry.message), "%s: %s", event, details);
    }
    entry.is_valid = true;

    return data_logger_add_entry(logger, &entry);
}

int data_logger_add_user_action(data_logger_t *logger, const char *action, const char *details)
{
    if (!logger || !action)
        return -1;

    log_entry_t entry = {0};
    entry.timestamp = data_logger_get_current_timestamp();
    entry.type = LOG_ENTRY_USER_ACTION;
    strncpy(entry.source, "User", sizeof(entry.source) - 1);
    strncpy(entry.message, action, sizeof(entry.message) - 1);
    if (details)
    {
        snprintf(entry.message, sizeof(entry.message), "%s: %s", action, details);
    }
    entry.is_valid = true;

    return data_logger_add_entry(logger, &entry);
}

int data_logger_add_error(data_logger_t *logger, const char *error, const char *details)
{
    if (!logger || !error)
        return -1;

    log_entry_t entry = {0};
    entry.timestamp = data_logger_get_current_timestamp();
    entry.type = LOG_ENTRY_ERROR;
    strncpy(entry.source, "Error", sizeof(entry.source) - 1);
    strncpy(entry.message, error, sizeof(entry.message) - 1);
    if (details)
    {
        snprintf(entry.message, sizeof(entry.message), "%s: %s", error, details);
    }
    entry.is_valid = true;

    return data_logger_add_entry(logger, &entry);
}

int data_logger_create_log_file(data_logger_t *logger)
{
    if (!logger)
        return -1;

    // Generate filename with timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    snprintf(logger->current_filename, sizeof(logger->current_filename),
             "%s/obd_log_%04d%02d%02d_%02d%02d%02d.%s",
             logger->config.log_directory,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             (logger->config.format == LOG_FORMAT_CSV) ? "csv" : (logger->config.format == LOG_FORMAT_JSON) ? "json"
                                                                                                            : "bin");

    // Open file
    logger->current_file = fopen(logger->current_filename, "w");
    if (!logger->current_file)
        return -1;

    // Write header based on format
    if (logger->config.format == LOG_FORMAT_CSV)
    {
        fprintf(logger->current_file, "Timestamp,Type,Source,Message,Value,Unit,DTC_Code,Valid\n");
    }
    else if (logger->config.format == LOG_FORMAT_JSON)
    {
        fprintf(logger->current_file, "[\n");
    }

    logger->statistics.files_created++;
    return 0;
}

int data_logger_close_log_file(data_logger_t *logger)
{
    if (!logger || !logger->current_file)
        return -1;

    // Close JSON array if needed
    if (logger->config.format == LOG_FORMAT_JSON)
    {
        fprintf(logger->current_file, "\n]\n");
    }

    fclose(logger->current_file);
    logger->current_file = NULL;

    return 0;
}

int data_logger_flush_log_file(data_logger_t *logger)
{
    if (!logger || !logger->current_file)
        return -1;

    fflush(logger->current_file);
    return 0;
}

int data_logger_write_entry_to_file(data_logger_t *logger, const log_entry_t *entry)
{
    if (!logger || !entry || !logger->current_file)
        return -1;

    switch (logger->config.format)
    {
    case LOG_FORMAT_CSV:
        fprintf(logger->current_file, "%llu,%d,%s,%s,%.2f,%s,%d,%s\n",
                (unsigned long long)entry->timestamp,
                entry->type,
                entry->source,
                entry->message,
                entry->value,
                entry->unit ? entry->unit : "",
                entry->dtc_code,
                entry->is_valid ? "true" : "false");
        break;

    case LOG_FORMAT_JSON:
        // Add comma if not first entry
        if (logger->statistics.total_entries > 1)
        {
            fprintf(logger->current_file, ",\n");
        }
        fprintf(logger->current_file, "  {\n");
        fprintf(logger->current_file, "    \"timestamp\": %llu,\n", (unsigned long long)entry->timestamp);
        fprintf(logger->current_file, "    \"type\": %d,\n", entry->type);
        fprintf(logger->current_file, "    \"source\": \"%s\",\n", entry->source);
        fprintf(logger->current_file, "    \"message\": \"%s\",\n", entry->message);
        fprintf(logger->current_file, "    \"value\": %.2f,\n", entry->value);
        fprintf(logger->current_file, "    \"unit\": \"%s\",\n", entry->unit ? entry->unit : "");
        fprintf(logger->current_file, "    \"dtc_code\": %d,\n", entry->dtc_code);
        fprintf(logger->current_file, "    \"valid\": %s\n", entry->is_valid ? "true" : "false");
        fprintf(logger->current_file, "  }");
        break;

    case LOG_FORMAT_BINARY:
        fwrite(entry, sizeof(log_entry_t), 1, logger->current_file);
        break;
    }

    return 0;
}

int data_logger_export_csv(data_logger_t *logger, const char *filename)
{
    if (!logger || !filename)
        return -1;

    FILE *file = fopen(filename, "w");
    if (!file)
        return -1;

    // Write CSV header
    fprintf(file, "Timestamp,Type,Source,Message,Value,Unit,DTC_Code,Valid\n");

    // Write entries
    for (size_t i = 0; i < logger->entry_count; i++)
    {
        const log_entry_t *entry = &logger->entries[i];
        fprintf(file, "%llu,%d,%s,%s,%.2f,%s,%d,%s\n",
                (unsigned long long)entry->timestamp,
                entry->type,
                entry->source,
                entry->message,
                entry->value,
                entry->unit ? entry->unit : "",
                entry->dtc_code,
                entry->is_valid ? "true" : "false");
    }

    fclose(file);
    return 0;
}

int data_logger_export_json(data_logger_t *logger, const char *filename)
{
    if (!logger || !filename)
        return -1;

    FILE *file = fopen(filename, "w");
    if (!file)
        return -1;

    fprintf(file, "[\n");

    for (size_t i = 0; i < logger->entry_count; i++)
    {
        const log_entry_t *entry = &logger->entries[i];

        if (i > 0)
            fprintf(file, ",\n");

        fprintf(file, "  {\n");
        fprintf(file, "    \"timestamp\": %llu,\n", (unsigned long long)entry->timestamp);
        fprintf(file, "    \"type\": %d,\n", entry->type);
        fprintf(file, "    \"source\": \"%s\",\n", entry->source);
        fprintf(file, "    \"message\": \"%s\",\n", entry->message);
        fprintf(file, "    \"value\": %.2f,\n", entry->value);
        fprintf(file, "    \"unit\": \"%s\",\n", entry->unit ? entry->unit : "");
        fprintf(file, "    \"dtc_code\": %d,\n", entry->dtc_code);
        fprintf(file, "    \"valid\": %s\n", entry->is_valid ? "true" : "false");
        fprintf(file, "  }");
    }

    fprintf(file, "\n]\n");
    fclose(file);
    return 0;
}

const log_entry_t *data_logger_get_entry(data_logger_t *logger, size_t index)
{
    if (!logger || index >= logger->entry_count)
        return NULL;
    return &logger->entries[index];
}

size_t data_logger_get_entry_count(data_logger_t *logger)
{
    return logger ? logger->entry_count : 0;
}

const log_statistics_t *data_logger_get_statistics(data_logger_t *logger)
{
    return logger ? &logger->statistics : NULL;
}

// Utility functions
const char *log_entry_type_to_string(log_entry_type_t type)
{
    switch (type)
    {
    case LOG_ENTRY_OBD_DATA:
        return "OBD Data";
    case LOG_ENTRY_DTC_CODE:
        return "DTC Code";
    case LOG_ENTRY_SYSTEM_EVENT:
        return "System Event";
    case LOG_ENTRY_USER_ACTION:
        return "User Action";
    case LOG_ENTRY_ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

const char *log_format_to_string(log_format_t format)
{
    switch (format)
    {
    case LOG_FORMAT_CSV:
        return "CSV";
    case LOG_FORMAT_JSON:
        return "JSON";
    case LOG_FORMAT_BINARY:
        return "Binary";
    default:
        return "Unknown";
    }
}

uint64_t data_logger_get_current_timestamp(void)
{
    return (uint64_t)time(NULL);
}

int data_logger_set_default_config(data_logger_t *logger)
{
    if (!logger)
        return -1;

    logger->config.auto_log = true;
    logger->config.log_interval_ms = 1000;
    logger->config.format = LOG_FORMAT_CSV;
    strcpy(logger->config.log_directory, "./logs");
    strcpy(logger->config.log_filename, "obd_log");
    logger->config.max_file_size_mb = 10;
    logger->config.max_files = 10;
    logger->config.compress_old_files = false;
    logger->config.include_timestamps = true;
    logger->config.include_metadata = true;

    return 0;
}

int data_logger_set_high_frequency_config(data_logger_t *logger)
{
    if (!logger)
        return -1;

    logger->config.auto_log = true;
    logger->config.log_interval_ms = 100;      // 10Hz logging
    logger->config.format = LOG_FORMAT_BINARY; // More efficient
    strcpy(logger->config.log_directory, "./logs");
    strcpy(logger->config.log_filename, "obd_log");
    logger->config.max_file_size_mb = 50;
    logger->config.max_files = 20;
    logger->config.compress_old_files = true;
    logger->config.include_timestamps = true;
    logger->config.include_metadata = true;

    return 0;
}

int data_logger_set_compact_config(data_logger_t *logger)
{
    if (!logger)
        return -1;

    logger->config.auto_log = true;
    logger->config.log_interval_ms = 5000; // 5 second intervals
    logger->config.format = LOG_FORMAT_CSV;
    strcpy(logger->config.log_directory, "./logs");
    strcpy(logger->config.log_filename, "obd_log");
    logger->config.max_file_size_mb = 5;
    logger->config.max_files = 5;
    logger->config.compress_old_files = false;
    logger->config.include_timestamps = true;
    logger->config.include_metadata = false;

    return 0;
}

int data_logger_set_debug_config(data_logger_t *logger)
{
    if (!logger)
        return -1;

    logger->config.auto_log = true;
    logger->config.log_interval_ms = 100;
    logger->config.format = LOG_FORMAT_JSON; // Human readable
    strcpy(logger->config.log_directory, "./logs");
    strcpy(logger->config.log_filename, "obd_debug");
    logger->config.max_file_size_mb = 100;
    logger->config.max_files = 50;
    logger->config.compress_old_files = false;
    logger->config.include_timestamps = true;
    logger->config.include_metadata = true;

    return 0;
}
