#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include "raylib.h"

// возвращает Color по индексу палитры (с защитой от выхода за границы)
Color liko4_get_palette(uint8_t idx);

// Загрузка шрифта
void liko4_load_font(const char *filepath);

void fps(int16_t x, int16_t y, uint8_t color);

void cls(uint8_t color);
void print(const char* format, int16_t x, int16_t y, int color, ...);
void pal(uint8_t c0, uint8_t c1);
void pal_reset(void);

// -- API для рисование примитивов
void circ(int16_t x, int16_t y, int16_t r, uint8_t color);
void circb(int16_t x, int16_t y, int16_t r, uint8_t color);
void rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void rectb(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void elli (int16_t x, int16_t y, int16_t a, int16_t b, uint8_t color);
void ellib(int16_t x, int16_t y, int16_t a, int16_t b, uint8_t color);
void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);

// --- API для ввода ---
bool btn(uint8_t id);
bool btnp(uint8_t id);

// --- API для мыши
typedef struct {
    int16_t x;         // координаты указателя в виртуальном экране (0..159)
    int16_t y;
    bool    left;      // нажата ли кнопка сейчас
    bool    middle;
    bool    right;
    int8_t  scrollx;   // прокрутка за кадр (-31..32)
    int8_t  scrolly;
} liko4_mouse;

liko4_mouse mouse(void);    // кнопки: held
liko4_mouse mousep(void);   // кнопки: pressed this frame

// --- API для время ---
int64_t liko4_utime(void);

void   liko4_time_reset(void);
double liko4_time(void);