#pragma once

// Открыть лог рядом с exe. Вызывать один раз в начале main.
void liko4_log_init(void);

// Закрыть лог. Вызывать перед выходом.
void liko4_log_close(void);

// Свои логи (аналог printf, но в файл).
void liko4_log(const char *format, ...);