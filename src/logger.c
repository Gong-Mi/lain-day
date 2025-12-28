#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static FILE* log_file = NULL;

void logger_init(const char* filename) {
    if (log_file) fclose(log_file);
    log_file = fopen(filename, "w"); // Overwrite each session for clarity
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", filename);
    } else {
        logger_log("Logger initialized.");
    }
}

void logger_log(const char* format, ...) {
    if (!log_file) return;

    // Get current time
    time_t rawtime;
    struct tm* timeinfo;
    char time_buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", timeinfo);

    fprintf(log_file, "[%s] ", time_buffer);

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file); // Ensure immediate write
}

void logger_close() {
    if (log_file) {
        logger_log("Logger closed.");
        fclose(log_file);
        log_file = NULL;
    }
}

