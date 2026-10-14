#pragma once
#include <stdint.h>
#include <stdbool.h>

// Инициализация
void liko4_save_init(const char *save_name);

// Чтение/запись (в память)
void    liko4_dset(int pos, int64_t value);
int64_t liko4_dget(int pos);

// Запись всего на диск
bool liko4_save_all(void);