#include "liko4_api_lua.h"
#include "liko4_api.h"
#include "liko4_sprites.h"
#include "liko4_audio.h"
#include "liko4_save.h"


// ============================================================
// Lua-биндинги
// ============================================================

static int l_cls(lua_State *Lstate) {
    int c = (int)luaL_checkinteger(Lstate, 1);
    cls((uint8_t)c);
    return 0;
}

static int l_print(lua_State *Lstate) {
    const char *text = luaL_checkstring(Lstate, 1);
    int x = (int)luaL_checkinteger(Lstate, 2);
    int y = (int)luaL_checkinteger(Lstate, 3);
    int c = (int)luaL_checkinteger(Lstate, 4);
    print("%s", (int16_t)x, (int16_t)y, c, text);
    return 0;
}

static int l_fps(lua_State *Lstate) {
    int x = (int)luaL_checkinteger(Lstate, 1);
    int y = (int)luaL_checkinteger(Lstate, 2);
    int c = (int)luaL_checkinteger(Lstate, 3);
    fps((int16_t)x, (int16_t)y, (uint8_t)c);
    return 0;
}

static int l_pal(lua_State *L) {
    int nargs = lua_gettop(L);

    if (nargs == 0) {
        // pal() — сброс на исходную палитру
        pal_reset();
        return 0;
    }

    if (nargs == 2) {
        // pal(c0, c1) — всё, что рисовалось цветом c0, теперь рисуется как c1
        int c0 = (int)luaL_checkinteger(L, 1);
        int c1 = (int)luaL_checkinteger(L, 2);
        pal((uint8_t)c0, (uint8_t)c1);
        return 0;
    }

    // Неверное число аргументов — ошибка в Lua
    return luaL_error(L, "pal() expects 0 or 2 arguments, got %d", nargs);
}

// -- Примитивы --
static int l_circ(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int r = (int)luaL_checkinteger(L, 3);
    int c = (int)luaL_checkinteger(L, 4);
    circ((int16_t)x, (int16_t)y, (int16_t)r, (uint8_t)c);
    return 0;
}

static int l_circb(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int r = (int)luaL_checkinteger(L, 3);
    int c = (int)luaL_checkinteger(L, 4);
    circb((int16_t)x, (int16_t)y, (int16_t)r, (uint8_t)c);
    return 0;
}

static int l_rect(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int w = (int)luaL_checkinteger(L, 3);
    int h = (int)luaL_checkinteger(L, 4);
    int c = (int)luaL_checkinteger(L, 5);
    rect((int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, (uint8_t)c);
    return 0;
}

static int l_rectb(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int w = (int)luaL_checkinteger(L, 3);
    int h = (int)luaL_checkinteger(L, 4);
    int c = (int)luaL_checkinteger(L, 5);
    rectb((int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, (uint8_t)c);
    return 0;
}

static int l_elli(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int a = (int)luaL_checkinteger(L, 3);
    int b = (int)luaL_checkinteger(L, 4);
    int c = (int)luaL_checkinteger(L, 5);
    elli((int16_t)x, (int16_t)y, (int16_t)a, (int16_t)b, (uint8_t)c);
    return 0;
}

static int l_ellib(lua_State *L) {
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int a = (int)luaL_checkinteger(L, 3);
    int b = (int)luaL_checkinteger(L, 4);
    int c = (int)luaL_checkinteger(L, 5);
    ellib((int16_t)x, (int16_t)y, (int16_t)a, (int16_t)b, (uint8_t)c);
    return 0;
}

static int l_line(lua_State *L) {
    int x0 = (int)luaL_checkinteger(L, 1);
    int y0 = (int)luaL_checkinteger(L, 2);
    int x1 = (int)luaL_checkinteger(L, 3);
    int y1 = (int)luaL_checkinteger(L, 4);
    int c  = (int)luaL_checkinteger(L, 5);
    line((int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1, (uint8_t)c);
    return 0;
}

// -- Кнопки --
static int l_btn(lua_State *L) {
    int key = luaL_checkinteger(L, 1);
    lua_pushboolean(L, btn((uint8_t)key));
    return 1;
}

static int l_btnp(lua_State *L) {
    int key = luaL_checkinteger(L, 1);
    lua_pushboolean(L, btnp((uint8_t)key));
    return 1;
}

// -- Мышка --
static void push_mouse_table(lua_State *L, liko4_mouse m) {
    lua_newtable(L);
    lua_pushinteger(L, m.x);        lua_setfield(L, -2, "x");
    lua_pushinteger(L, m.y);        lua_setfield(L, -2, "y");
    lua_pushboolean(L, m.left);     lua_setfield(L, -2, "left");
    lua_pushboolean(L, m.middle);   lua_setfield(L, -2, "middle");
    lua_pushboolean(L, m.right);    lua_setfield(L, -2, "right");
    lua_pushinteger(L, m.scrollx);  lua_setfield(L, -2, "scrollx");
    lua_pushinteger(L, m.scrolly);  lua_setfield(L, -2, "scrolly");
}

static int l_mouse(lua_State *L) {
    push_mouse_table(L, mouse());
    return 1;
}

static int l_mousep(lua_State *L) {
    push_mouse_table(L, mousep());
    return 1;
}

// Спрайты
static int l_spr(lua_State *L) {
    int n = (int)luaL_checkinteger(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);

    // опциональные: по умолчанию 1×1 спрайт
    int w = (int)luaL_optinteger(L, 4, 1);
    int h = (int)luaL_optinteger(L, 5, 1);

    // опциональные: по умолчанию без флипа
    bool fx = lua_toboolean(L, 6) != 0;
    bool fy = lua_toboolean(L, 7) != 0;

    spr_pro((int16_t)n, (int16_t)x, (int16_t)y,
            (uint8_t)w, (uint8_t)h, fx, fy);
    return 0;
}

// Звук
static int l_sfx(lua_State *L) {
    int idx = (int)luaL_checkinteger(L, 1);
    if (lua_gettop(L) >= 2) {
        float vol = (float)luaL_checknumber(L, 2);
        sfx_ex(idx, vol);
    } else {
        sfx_ex(idx, 1.0f); // 100% громкости
    }
    return 0;
}

// Музыка
static int l_music(lua_State *L) {
    int nargs = lua_gettop(L);

    if (nargs == 0) {
        music_stop();
        return 0;
    }

    int idx = (int)luaL_checkinteger(L, 1);
    music(idx);

    if (nargs >= 2) {
        float vol = (float)luaL_checknumber(L, 2);
        music_volume(vol);
    }

    return 0;
}

// Тона
static int l_tone(lua_State *L) {
    int freq     = (int)luaL_checkinteger(L, 1);
    int duration = (int)luaL_checkinteger(L, 2);
    int volume   = (int)luaL_checkinteger(L, 3);
    int flags    = (int)luaL_optinteger(L, 4, 0);
    tone(freq, duration, volume, flags);
    return 0;
}

// Сохранения
static int l_dset(lua_State *L) {
    int pos = (int)luaL_checkinteger(L, 1);
    lua_Integer val = luaL_checkinteger(L, 2);
    liko4_dset(pos, (int64_t)val);
    return 0;
}

static int l_dget(lua_State *L) {
    int pos = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer)liko4_dget(pos));
    return 1;
}

// Время
static int l_utime(lua_State *L) {
    lua_pushinteger(L, (lua_Integer)liko4_utime());
    return 1;
}

static int l_time(lua_State *L) {
    lua_pushnumber(L, liko4_time());
    return 1;
}

// ============================================================
// Регистрация всех биндингов в глобальную область
// ============================================================

static void reg(lua_State *L, const char *name, lua_CFunction fn) {
    lua_pushcfunction(L, fn);
    lua_setglobal(L, name);
}

void liko4_register_api(lua_State *Lstate) {
    // графика
    reg(Lstate, "cls",    l_cls);
    reg(Lstate, "print",  l_print);
    reg(Lstate, "fps",    l_fps);
    reg(Lstate, "pal",    l_pal);

    // примитивы
    reg(Lstate, "circ",   l_circ);
    reg(Lstate, "circb",  l_circb);
    reg(Lstate, "rect",   l_rect);
    reg(Lstate, "rectb",  l_rectb);
    reg(Lstate, "elli",   l_elli);
    reg(Lstate, "ellib",  l_ellib);
    reg(Lstate, "line",   l_line);

    // спрайты
    reg(Lstate, "spr",    l_spr);

    // ввод
    reg(Lstate, "btn",    l_btn);
    reg(Lstate, "btnp",   l_btnp);
    reg(Lstate, "mouse",  l_mouse);
    reg(Lstate, "mousep", l_mousep);

    // время
    reg(Lstate, "utime",  l_utime);
    reg(Lstate, "time",   l_time);

    // звук
    reg(Lstate, "sfx",    l_sfx);
    reg(Lstate, "music",  l_music);
    reg(Lstate, "tone",   l_tone);

    // сохранения
    reg(Lstate, "dset",   l_dset);
    reg(Lstate, "dget",   l_dget);
}