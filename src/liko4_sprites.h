#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"

// Загрузить спрайт-лист (PNG 256×256) и построить индексный буфер.
void liko4_load_spritesheet(const char *filepath);

// Перестроить текстуру из индексов и текущей палитры.
// Вызывается автоматически при set_pal(). Можно звать вручную.
void liko4_rebuild_spritesheet(void);

void spr_pro(int16_t n, int16_t x, int16_t y,
             uint8_t cols, uint8_t rows,
             bool flip_x, bool flip_y);

void spr(int16_t n, int16_t x, int16_t y);