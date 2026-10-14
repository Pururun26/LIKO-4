#include "liko4_api.h"
#include "liko4_api_lua.h"
#include "liko4_conf.h"
#include "liko4_log.h"
#include "liko4_sprites.h"
#include "liko4_audio.h"
#include "liko4_save.h"

#ifndef __EMSCRIPTEN__
#include "liko4_cart.h"
#endif

#include "raylib.h"
#include <stdio.h>
#include <string.h>

static RenderTexture2D target;
static lua_State *L = NULL;

char g_data_dir[1024] = {0};

// ============================================================
// Вызов Lua-функций из C
// ============================================================

static void call_lua(const char *name) {
    if (!L) return;

    lua_getglobal(L, name);

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        liko4_log("lua error in %s: %s", name, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

// ============================================================
// Настройка package.path по папке игры
// ============================================================
static void setup_lua_path(lua_State *Lstate, const char *game_path) {
    const char *slash  = strrchr(game_path, '/');
    const char *bslash = strrchr(game_path, '\\');
    if (bslash && (!slash || bslash > slash)) slash = bslash;

    if (!slash) return;   // игра в текущей папке — package.path уже рабочий

    int dir_len = (int)(slash - game_path);
    char lua_path[1024];
    snprintf(lua_path, sizeof(lua_path),
             "%.*s/?.lua;%.*s/?/init.lua;./?.lua",
             dir_len, game_path,
             dir_len, game_path);

    lua_getglobal(Lstate, "package");
    lua_pushstring(Lstate, lua_path);
    lua_setfield(Lstate, -2, "path");
    lua_pop(Lstate, 1);
}


// ============================================================
// Подсчёт символов во всех .lua файлах директории (рекурсивно)
// ============================================================
static long count_lua_chars(const char *dir) {
    if (!DirectoryExists(dir)) return 0;

    FilePathList entries = LoadDirectoryFiles(dir);
    long total = 0;

    for (unsigned i = 0; i < entries.count; i++) {
        if (DirectoryExists(entries.paths[i])) {
            total += count_lua_chars(entries.paths[i]);
        } else {
            size_t len = strlen(entries.paths[i]);
            if (len >= 4 && strcmp(entries.paths[i] + len - 4, ".lua") == 0) {
                int size = GetFileLength(entries.paths[i]);
                if (size > 0) total += size;
            }
        }
    }

    UnloadDirectoryFiles(entries);
    return total;
}

// Возвращает директорию, в которой лежит game_path (директорию скриптов)
static void get_scripts_dir(const char *game_path, char *out, size_t out_size) {
    const char *slash  = strrchr(game_path, '/');
    const char *bslash = strrchr(game_path, '\\');
    if (bslash && (!slash || bslash > slash)) slash = bslash;

    if (!slash) {
        snprintf(out, out_size, ".");
        return;
    }

    int dir_len = (int)(slash - game_path);
    snprintf(out, out_size, "%.*s", dir_len, game_path);
}


// ============================================================
// Перезагрузка игры (Ctrl+R)
//   Перезагружает: Lua, спрайты, шрифт, звуки.
//   НЕ трогает: окно, storage.bin, рендер-текстуру.
// ============================================================
static void reload_game(const char *game_path) {
    liko4_log("LIKO-4: Reloading...");

    // 0. Проверка лимита символов скриптов (ДО закрытия старого Lua)
    {
        char scripts_dir[1024];
        get_scripts_dir(game_path, scripts_dir, sizeof(scripts_dir));

        long total = count_lua_chars(scripts_dir);
        if (total > LIKO4_MAX_SCRIPT_CHARS) {
            liko4_log("LIKO-4: script too large — %ld / %d chars (over by %ld)",
                      total, LIKO4_MAX_SCRIPT_CHARS,
                      total - LIKO4_MAX_SCRIPT_CHARS);
            return;   // старая игра остаётся работать
        }
        liko4_log("LIKO-4: script size %ld / %d chars",
                  total, LIKO4_MAX_SCRIPT_CHARS);
    }

    // 1. Закрыть старый Lua, если был
    if (L) {
        lua_close(L);
        L = NULL;
    }

    // 2. Перезагрузить спрайты
    {
        char path[1024];
        snprintf(path, sizeof(path),
                 "%s%s", g_data_dir, LIKO4_SPRITESHEET_PATH);
        liko4_load_spritesheet(path);
    }

    // 3. Перезагрузить шрифт
    {
        char path[1024];
        snprintf(path, sizeof(path),
                 "%s%s", g_data_dir, LIKO4_FONT_PATH);
        liko4_load_font(path);
    }

    // 4. Перезагрузить звуки и музыку
    liko4_audio_close();
    liko4_audio_init(g_data_dir);

    liko4_time_reset();

    // 5. Создать новый Lua
    L = luaL_newstate();

    // Открываем только безопасные библиотеки
    luaL_requiref(L, "_G",        luaopen_base,      1); lua_pop(L, 1);
    luaL_requiref(L, "math",      luaopen_math,      1); lua_pop(L, 1);
    luaL_requiref(L, "string",    luaopen_string,    1); lua_pop(L, 1);
    luaL_requiref(L, "table",     luaopen_table,     1); lua_pop(L, 1);
    luaL_requiref(L, "coroutine", luaopen_coroutine, 1); lua_pop(L, 1);
    luaL_requiref(L, "utf8",      luaopen_utf8,      1); lua_pop(L, 1);
    luaL_requiref(L, "package",   luaopen_package,   1); lua_pop(L, 1);
    // НЕ открываем: io, os, debug
    liko4_register_api(L);
    setup_lua_path(L, game_path);

    if (luaL_dofile(L, game_path) != LUA_OK) {
        liko4_log("lua error: %s", lua_tostring(L, -1));
        lua_close(L);
        L = NULL;
        // Игра не запущена, но окно живо — можно править и жать Ctrl+R снова
    } else {
        call_lua("_init");
        liko4_log("LIKO-4: Reloaded.");
    }
}

// ============================================================
// main
// ============================================================
int main(int argc, char **argv) {
    // ============================================================
    // 1. Логирование
    // ============================================================
    liko4_log_init();

    const char *exe_dir = GetApplicationDirectory();

    char game_path[1024]     = {0};
    char cart_out[1024]      = {0};
    char cart_path[1024]     = {0};

    // ============================================================
    // 2. Определяем источник игры
    //    Desktop:  1) аргумент .lk4   2) game.lk4 рядом с exe   3) data/
    //    Web:      JS уже распаковал в /data/, идём сразу в dev
    // ============================================================

#ifndef __EMSCRIPTEN__
    if (argc >= 2) {
        snprintf(cart_path, sizeof(cart_path), "%s", argv[1]);
        liko4_log("LIKO-4: cart from argument: %s", cart_path);
    } else {
        char candidate[1024];
        snprintf(candidate, sizeof(candidate), "%sgame.lk4", exe_dir);
        if (FileExists(candidate)) {
            snprintf(cart_path, sizeof(cart_path), "%s", candidate);
            liko4_log("LIKO-4: found game.lk4");
        }
    }

    if (cart_path[0]) {
        // ─── Картридж найден — распаковываем в _cart/ ───
        snprintf(cart_out, sizeof(cart_out), "%s_cart", exe_dir);

        if (!liko4_cart_extract(cart_path, cart_out)) {
            liko4_log("LIKO-4: failed to extract '%s'", cart_path);
            liko4_log_close();
            return 1;
        }

        snprintf(game_path, sizeof(game_path),
                 "%s/data/scripts/main.lua", cart_out);
        snprintf(g_data_dir, sizeof(g_data_dir), "%s/", cart_out);
    } else
#endif
    {
        // ─── Режим разработки — data/ рядом с exe ───
        char data_dir[1024];
        snprintf(data_dir, sizeof(data_dir), "%sdata", exe_dir);

        if (!DirectoryExists(data_dir)) {
            liko4_log("LIKO-4: no game.lk4 and no data/ found.");
            liko4_log_close();
            return 1;
        }

        snprintf(game_path, sizeof(game_path),
                 "%s%s", exe_dir, LIKO4_MAIN_SCRIPT);
        snprintf(g_data_dir, sizeof(g_data_dir), "%s", exe_dir);
        liko4_log("LIKO-4: using data/ directly (dev mode)");
    }

    liko4_log("game path: %s", game_path);

    char save_name[128] = "storage";

#ifndef __EMSCRIPTEN__
    if (cart_path[0]) {
        const char *base = cart_path;
        const char *s1 = strrchr(cart_path, '/');
        const char *s2 = strrchr(cart_path, '\\');
        if (s2 && (!s1 || s2 > s1)) base = s2 + 1;
        else if (s1)               base = s1 + 1;

        snprintf(save_name, sizeof(save_name), "%s", base);

        // Отрезаем расширение
        char *dot = strrchr(save_name, '.');
        if (dot) *dot = '\0';

        if (!save_name[0]) snprintf(save_name, sizeof(save_name), "storage");
    }
#endif

    // ============================================================
    // 3. Raylib
    // ============================================================
    InitWindow(LIKO4_WINDOW_WIDTH, LIKO4_WINDOW_HEIGHT, LIKO4_TITLE);

    HideCursor();
    SetTargetFPS(LIKO4_FPS);

    target = LoadRenderTexture(LIKO4_WIDTH, LIKO4_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    liko4_save_init(save_name);

    // ============================================================
    // 4. Первая загрузка игры
    // ============================================================
    reload_game(game_path);

    if (!L) {
        liko4_log("LIKO-4: Game failed to load. Press Ctrl+R to reload.");
    }

    int autosave_counter = 0;
    const int AUTOSAVE_INTERVAL = 60 * 60;

    // ============================================================
    // 5. Главный цикл
    // ============================================================
    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R)) {
            reload_game(game_path);
        }

        music_update();

        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        autosave_counter++;
        if (autosave_counter >= AUTOSAVE_INTERVAL) {
            liko4_save_all();
            autosave_counter = 0;
        }

        call_lua("_update");

        BeginTextureMode(target);
            ClearBackground(BLACK);
            call_lua("_draw");
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);

            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();

            int scale_x = screenW / LIKO4_WIDTH;
            int scale_y = screenH / LIKO4_HEIGHT;
            int scale   = (scale_x < scale_y) ? scale_x : scale_y;
            if (scale < 1) scale = 1;

            int draw_w = LIKO4_WIDTH  * scale;
            int draw_h = LIKO4_HEIGHT * scale;
            int offsetX = (screenW - draw_w) / 2;
            int offsetY = (screenH - draw_h) / 2;

            DrawTexturePro(
                target.texture,
                (Rectangle){ 0, 0, (float)LIKO4_WIDTH, (float)-LIKO4_HEIGHT },
                (Rectangle){ (float)offsetX, (float)offsetY,
                            (float)draw_w, (float)draw_h },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE
            );
        EndDrawing();
    }

    // ============================================================
    // 6. Завершение
    // ============================================================
    liko4_log("closing");

    liko4_save_all();
    liko4_audio_close();
    UnloadRenderTexture(target);

#ifndef __EMSCRIPTEN__
    if (cart_out[0]) {
        liko4_cart_remove(cart_out);
    }
#endif

    CloseWindow();

    if (L) lua_close(L);

    liko4_log_close();
    return 0;
}