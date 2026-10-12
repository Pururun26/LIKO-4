#include <time.h>
#include "raylib.h"
#include "liko4_api.h"
#include "liko4_conf.h"
#include "liko4_sprites.h"

// ============================================================
// Палитра
// ============================================================

// Эталонные цвета — не меняются никогда.
static const Color palette_default[LIKO4_COLOR_COUNT] = {
    {0x00, 0x00, 0x00, 0xFF}, // 0: black
    {0x54, 0x4E, 0x5E, 0xFF}, // 1: gray
    {0x4D, 0x8E, 0xD9, 0xFF}, // 2: blue
    {0x00, 0xD4, 0x3B, 0xFF}, // 3: green
    {0xFF, 0xE4, 0xD2, 0xFF}, // 4: cream
    {0xFF, 0xBB, 0x0D, 0xFF}, // 5: amber
    {0xFF, 0x3B, 0x7A, 0xFF}, // 6: pink
    {0x91, 0x3F, 0x33, 0xFF}, // 7: brown
};

// Активные цвета, которые уходят в шейдер.
static Color palette[LIKO4_COLOR_COUNT] = {
    {0x00, 0x00, 0x00, 0xFF},
    {0x54, 0x4E, 0x5E, 0xFF},
    {0x4D, 0x8E, 0xD9, 0xFF},
    {0x00, 0xD4, 0x3B, 0xFF},
    {0xFF, 0xE4, 0xD2, 0xFF},
    {0xFF, 0xBB, 0x0D, 0xFF},
    {0xFF, 0x3B, 0x7A, 0xFF},
    {0x91, 0x3F, 0x33, 0xFF},
};

// Состояние: color_replace[draw_idx] = источник в palette_default.
// По умолчанию — identity (0→0, 1→1, ...).
static uint8_t color_replace[LIKO4_COLOR_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7 };

// Пересобрать активную палитру из эталонной по color_replace.
static void rebuild_active_palette(void) {
    for (int i = 0; i < LIKO4_COLOR_COUNT; i++) {
        uint8_t src = color_replace[i];
        if (src >= LIKO4_COLOR_COUNT) src = i;   // защита от мусора
        palette[i] = palette_default[src];
    }
}

// ============================================================
// Публичный API
// ============================================================

Color liko4_get_palette(uint8_t idx) {
    return (idx < LIKO4_COLOR_COUNT) ? palette[idx] : palette[0];
}

// pal(c0, c1) — как в PICO-8:
// всё, что рисовалось цветом c0, теперь рисуется цветом c1.
// Пример: pal(0, 1) — чёрный станет серым.
void pal(uint8_t c0, uint8_t c1) {
    if (c0 >= LIKO4_COLOR_COUNT) return;
    if (c1 >= LIKO4_COLOR_COUNT) return;

    color_replace[c0] = c1;
    rebuild_active_palette();
    liko4_rebuild_spritesheet();
}

// pal() без аргументов — сброс всех замен.
void pal_reset(void) {
    for (int i = 0; i < LIKO4_COLOR_COUNT; i++) {
        color_replace[i] = i;
    }
    rebuild_active_palette();
    liko4_rebuild_spritesheet();
}

// --- Шрифт ---
static Font  pico_font;
static bool  font_loaded = false;
static float font_size = 5.0f;
static float font_spacing = 1.0f;

void liko4_load_font(const char* filepath) {
    Image img = LoadImage(filepath);
    if (img.data != NULL) {
        pico_font = LoadFontFromImage(img, MAGENTA, 32);
        UnloadImage(img);
        font_loaded = true;
        TraceLog(LOG_INFO, "LIKO-4: Font '%s' loaded successfully.", filepath);
    } else {
        pico_font = GetFontDefault();
        font_size = 10.0f;
        font_loaded = false;
        TraceLog(LOG_WARNING, "LIKO-4: Font '%s' not found. Using default.", filepath);
    }
}

// ============================================================
// C-функции API (используются Lua-биндингами)
// ============================================================

void cls(uint8_t color) {
    if (color < LIKO4_COLOR_COUNT) ClearBackground(palette[color]);
    else                             ClearBackground(palette[0]);
}

void print(const char* format, int16_t x, int16_t y, int color, ...) {
    va_list args;
    va_start(args, color);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Color c = (color < LIKO4_COLOR_COUNT) ? palette[color] : palette[0];
    Vector2 pos = { (float)x, (float)y };
    DrawTextEx(pico_font, buffer, pos, font_size, font_spacing, c);
}

void fps(int16_t x, int16_t y, uint8_t color) {
    int value = GetFPS();
    print("FPS %d", x, y, color, value);
}

// --- ПРИМИТИВЫ РИСОВАНИЯ ---
void circ(int16_t x, int16_t y, int16_t r, uint8_t color) {
    DrawCircle(x, y, (float)r, liko4_get_palette(color));
}

void circb(int16_t x, int16_t y, int16_t r, uint8_t color) {
    DrawCircleLines(x, y, (float)r, liko4_get_palette(color));
}

void rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    DrawRectangle(x, y, w, h, liko4_get_palette(color));
}

void rectb(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    DrawRectangleLines(x, y, w, h, liko4_get_palette(color));
}

void elli(int16_t x, int16_t y, int16_t a, int16_t b, uint8_t color) {
    if (a <= 0 || b <= 0) return;
    DrawEllipse(x, y, (float)a, (float)b, liko4_get_palette(color));
}

void ellib(int16_t x, int16_t y, int16_t a, int16_t b, uint8_t color) {
    if (a <= 0 || b <= 0) return;
    DrawEllipseLines(x, y, (float)a, (float)b, liko4_get_palette(color));
}

// Рисует линию от (x0, y0) до (x1, y1)
void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    DrawLine(x0, y0, x1, y1, liko4_get_palette(color));
}

// --- API для ввода ---
bool btn(uint8_t id)
{
    switch (id)
    {
        case 0: return IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
        case 1: return IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
        case 2: return IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
        case 3: return IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
        case 4: return IsKeyDown(KEY_J) || IsKeyDown(KEY_Z);
        case 5: return IsKeyDown(KEY_K) || IsKeyDown(KEY_X);
        default: return false;
    }
}

bool btnp(uint8_t id)
{
    switch (id)
    {
        case 0: return IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
        case 1: return IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
        case 2: return IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
        case 3: return IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
        case 4: return IsKeyPressed(KEY_J) || IsKeyPressed(KEY_Z);
        case 5: return IsKeyPressed(KEY_K) || IsKeyPressed(KEY_X);
        default: return false;
    }
}

// Мышь — перевод экранных координат в виртуальные
static liko4_mouse get_mouse_state(bool pressed) {
    liko4_mouse m = {0};

    // Экранные координаты
    Vector2 pos = GetMousePosition();

    // Масштаб и offset (как в главном цикле)
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    float scaleX = (float)screenW / LIKO4_WIDTH;
    float scaleY = (float)screenH / LIKO4_HEIGHT;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    int offsetX = (int)((screenW - LIKO4_WIDTH  * scale) / 2);
    int offsetY = (int)((screenH - LIKO4_HEIGHT * scale) / 2);

    // Перевод в виртуальные координаты
    m.x = (int16_t)((pos.x - offsetX) / scale);
    m.y = (int16_t)((pos.y - offsetY) / scale);

    // Кнопки: down или pressed
    if (pressed) {
        m.left   = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        m.middle = IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
        m.right  = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    } else {
        m.left   = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        m.middle = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        m.right  = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    }

    // Прокрутка с клампом в int8_t
    Vector2 w = GetMouseWheelMoveV();
    float wx = w.x;
    float wy = w.y;

    if (wx >  32) wx =  32;
    if (wx < -32) wx = -32;
    if (wy >  32) wy =  32;
    if (wy < -32) wy = -32;

    m.scrollx = (int8_t)wx;
    m.scrolly = (int8_t)wy;

    return m;
}

liko4_mouse mouse(void) {
    return get_mouse_state(false);
}

liko4_mouse mousep(void) {
    return get_mouse_state(true);
}

// --- API для время ---
int64_t liko4_utime(void) {
    return (int64_t)time(NULL);
}


static double liko4_start_time = 0.0;

void liko4_time_reset(void) {
    liko4_start_time = GetTime();
}

double liko4_time(void) {
    return (GetTime() - liko4_start_time) * 1000.0; // миллисекунды с запуска
}