#pragma once

// Настройки по умолчанию (пользователь может отредактировать)
// Размер окна (если не определены, используется автоматический расчёт)
#define LIKO4_WINDOW_WIDTH  512
#define LIKO4_WINDOW_HEIGHT 512
#define LIKO4_WIDTH   128
#define LIKO4_HEIGHT  128
#define LIKO4_TITLE   "LIKO-4"
#define LIKO4_FPS     60


// Палитра
#define LIKO4_COLOR_COUNT 8


// Скрипт игры
#define LIKO4_MAIN_SCRIPT  "data/scripts/main.lua"


// Спрайты
#define LIKO4_SPRITE_SIZE          8      // 8×8 пикселей
#define LIKO4_SPRITE_SHEET_TILES   16     // 32 спрайта в строке
#define LIKO4_SPRITE_SHEET_W       128    // ширина листа в пикселях
#define LIKO4_SPRITE_SHEET_H       128    // высота листа

#define LIKO4_SPRITESHEET_PATH     "data/sprites/spritesheet.png"


// Шрифт
#define LIKO4_FONT_PATH    "data/sprites/font.png"


// Звук
#define LIKO4_SOUNDS_PATH  "data/sounds/"
#define LIKO4_MUSIC_PATH   "data/music/"

#define LIKO4_MAX_SFX     32    // 0.wav … 31.wav
#define LIKO4_MAX_MUSIC    4    // 0.ogg … 3.ogg

// Сохранение и загрузка
#define LIKO4_SAVE_SLOTS 64