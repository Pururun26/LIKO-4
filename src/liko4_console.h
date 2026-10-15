#pragma once

// ============================================================
// Работа с консолью на Windows для GUI-приложений.
// Позволяет printf работать, когда exe собран с -mwindows.
// На Linux / macOS / Web — пустышки, ничего не делают.
// ============================================================

#ifdef _WIN32

// Отсекаем куски windows.h, которые конфликтуют с raylib:
//   WIN32_LEAN_AND_MEAN — убирает GDI, shellapi, winsock и т.д.
//   NOGDI               — убирает Rectangle (конфликт с raylib)
//   NOUSER              — убирает CloseWindow, ShowCursor (конфликты)
//   NOMINMAX            — убирает макросы min/max (конфликт с C++)
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NOMINMAX

#include <windows.h>
#include <stdio.h>

static void liko4_console_attach(void) {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$",  "r", stdin);
    }
}

static void liko4_console_detach(void) {
    FreeConsole();
}

#else
static void liko4_console_attach(void) { }
static void liko4_console_detach(void) { }
#endif