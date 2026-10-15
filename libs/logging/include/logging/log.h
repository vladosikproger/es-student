#pragma once
#include <stdio.h>

// Уровни важности сообщений
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3

// Текущий уровень (печатаем всё, что INFO и выше)
#define LOG_LEVEL LOG_LEVEL_INFO

#define LOG_ERR(fmt, ...) \
    do { if (LOG_LEVEL >= LOG_LEVEL_ERROR) { \
        log_prefix("[ERR]", __func__, __LINE__); \
        printf(fmt, ##__VA_ARGS__); \
    } } while (0)

#define LOG_INF(fmt, ...) \
    do { if (LOG_LEVEL >= LOG_LEVEL_INFO) { \
        log_prefix("[INF]", __func__, __LINE__); \
        printf(fmt, ##__VA_ARGS__); \
    } } while (0)

#define LOG_DBG(fmt, ...) \
    do { if (LOG_LEVEL >= LOG_LEVEL_DEBUG) { \
        log_prefix("[DBG]", __func__, __LINE__); \
        printf(fmt, ##__VA_ARGS__); \
    } } while (0)

void log_prefix(const char *level, const char *function, int line);