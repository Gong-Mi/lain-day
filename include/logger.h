#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdarg.h>

// Initialize the logger (opens the file)
void logger_init(const char* filename);

// Log a formatted message
void logger_log(const char* format, ...);

// Close the logger
void logger_close();

// --- Debug Macros ---

#ifdef USE_DEBUG_LOGGING
    #define LOG_DEBUG(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_DEBUG(fmt, ...)
#endif

#ifdef USE_STRING_DEBUG_LOGGING
    #define LOG_STRING_DEBUG(fmt, ...) fprintf(stderr, "DEBUG [STRING]: " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_STRING_DEBUG(fmt, ...)
#endif

#ifdef USE_MAP_DEBUG_LOGGING
    #define LOG_MAP_DEBUG(fmt, ...) fprintf(stderr, "DEBUG [MAP]: " fmt "\n", ##__VA_ARGS__)
#else
    #define LOG_MAP_DEBUG(fmt, ...)
#endif

#endif // LOGGER_H
