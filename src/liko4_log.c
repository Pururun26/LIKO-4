#include "liko4_log.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifndef __EMSCRIPTEN__
static FILE *log_file = NULL;
#endif

// ------------------------------------------------------------
// Уровень лога → строка
// ------------------------------------------------------------
static const char *level_name(int logLevel) {
    switch (logLevel) {
        case LOG_TRACE:   return "TRACE";
        case LOG_DEBUG:   return "DEBUG";
        case LOG_INFO:    return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR:   return "ERROR";
        case LOG_FATAL:   return "FATAL";
        default:          return "INFO";
    }
}

// ------------------------------------------------------------
// Перехват логов raylib (TraceLog)
// ------------------------------------------------------------
static void log_callback(int logLevel, const char *text, va_list args) {
    const char *level = level_name(logLevel);

#ifdef __EMSCRIPTEN__
    // Web: пишем в консоль браузера
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);

    EM_ASM({
        var lvl = UTF8ToString($0);
        var msg = UTF8ToString($1);
        if (lvl === 'ERROR' || lvl === 'FATAL' || lvl === 'WARNING') {
            console.error('[LIKO-4 ' + lvl + '] ' + msg);
        } else {
            console.log('[LIKO-4 ' + lvl + '] ' + msg);
        }
    }, level, buffer);
#else
    // Desktop: пишем в файл
    if (!log_file) return;

    fprintf(log_file, "[%s] ", level);
    vfprintf(log_file, text, args);
    fputc('\n', log_file);
    fflush(log_file);
#endif
}

// ------------------------------------------------------------
// API
// ------------------------------------------------------------
void liko4_log_init(void) {
#ifdef __EMSCRIPTEN__
    // Web: файлы не нужны, всё в console
    EM_ASM({
        console.log('=== LIKO-4 web ===');
    });
    SetTraceLogCallback(log_callback);
#else
    char path[1024];
    snprintf(path, sizeof(path), "%sliko4_log.txt", GetApplicationDirectory());

    log_file = fopen(path, "w");
    if (!log_file) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log_file, "=== LIKO-4 %04d-%02d-%02d %02d:%02d:%02d ===\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    fflush(log_file);

    SetTraceLogCallback(log_callback);
#endif
}

void liko4_log_close(void) {
#ifndef __EMSCRIPTEN__
    if (!log_file) return;
    fprintf(log_file, "=== log closed ===\n");
    fclose(log_file);
    log_file = NULL;
#endif
}

void liko4_log(const char *format, ...) {
    va_list args;
    va_start(args, format);

#ifdef __EMSCRIPTEN__
    // Web: console.log
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);

    EM_ASM({
        var msg = UTF8ToString($0);
        console.log('[LIKO-4] ' + msg);
    }, buffer);
#else
    // Desktop: файл
    if (log_file) {
        vfprintf(log_file, format, args);
        fputc('\n', log_file);
        fflush(log_file);
    }
#endif

    va_end(args);
}