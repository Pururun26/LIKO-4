#include "liko4_save.h"
#include "liko4_conf.h"
#include "raylib.h"

#include <string.h>
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// ============================================================
// Web: доступ к localStorage через EM_JS-функции
// ============================================================

EM_JS(int, js_localstorage_load, (void *ptr, int size), {
    var b64 = localStorage.getItem('storage');
    if (!b64) return 0;

    try {
        var bin = atob(b64);
        if (bin.length !== size) return 0;

        var bytes = new Uint8Array(bin.length);
        for (var i = 0; i < bin.length; i++) {
            bytes[i] = bin.charCodeAt(i);
        }
        HEAPU8.set(bytes, ptr);
        return 1;
    } catch (e) {
        console.error('[LIKO-4] localStorage read failed:', e);
        return 0;
    }
});

EM_JS(int, js_localstorage_save, (void *ptr, int size), {
    try {
        var bytes = HEAPU8.subarray(ptr, ptr + size);
        var bin = "";
        for (var i = 0; i < bytes.length; i++) {
            bin += String.fromCharCode(bytes[i]);
        }
        localStorage.setItem('storage', btoa(bin));
        return 1;
    } catch (e) {
        console.error('[LIKO-4] localStorage write failed:', e);
        return 0;
    }
});

#endif

// ============================================================
// Состояние
// ============================================================

static int64_t storage_data[LIKO4_SAVE_SLOTS];
static bool    storage_loaded = false;

#ifndef __EMSCRIPTEN__
static char save_path[1088];
static char save_name_global[128] = "storage";
#endif

// ============================================================
// Инициализация
// ============================================================

void liko4_save_init(const char *save_name) {
#ifdef __EMSCRIPTEN__
    // Web: читаем из localStorage под ключом 'storage'
    memset(storage_data, 0, sizeof(storage_data));

    int loaded = js_localstorage_load(storage_data, (int)sizeof(storage_data));

    if (loaded) {
        TraceLog(LOG_INFO, "LIKO-4: Save loaded from localStorage.");
    } else {
        TraceLog(LOG_INFO, "LIKO-4: No save, starting fresh.");
    }

#else
    // Запомнить имя для ленивой инициализации
    if (save_name && save_name[0]) {
        strncpy(save_name_global, save_name, sizeof(save_name_global) - 1);
        save_name_global[sizeof(save_name_global) - 1] = '\0';
    }
    
    // Native: папка save/ рядом с exe
    char save_dir[1024];
    snprintf(save_dir, sizeof(save_dir),
             "%ssave", GetApplicationDirectory());

    if (!DirectoryExists(save_dir)) {
        MakeDirectory(save_dir);
    }

    snprintf(save_path, sizeof(save_path),
             "%s/%s.bin", save_dir, save_name_global);

    int size = 0;
    unsigned char *data = NULL;

    if (FileExists(save_path)) {
        data = LoadFileData(save_path, &size);
    }

    if (data && size == (int)sizeof(storage_data)) {
        memcpy(storage_data, data, sizeof(storage_data));
        UnloadFileData(data);
        TraceLog(LOG_INFO, "LIKO-4: Save loaded from '%s'.", save_path);
    } else {
        if (data) UnloadFileData(data);
        memset(storage_data, 0, sizeof(storage_data));
        TraceLog(LOG_INFO, "LIKO-4: No save, starting fresh.");
    }
#endif

    storage_loaded = true;
}

// ============================================================
// Публичный API
// ============================================================

void liko4_dset(int pos, int64_t value) {
    if (pos < 0 || pos >= LIKO4_SAVE_SLOTS) return;
    if (!storage_loaded) liko4_save_init(NULL);
    storage_data[pos] = value;

#ifdef __EMSCRIPTEN__
    js_localstorage_save(storage_data, (int)sizeof(storage_data));
#endif
}

int64_t liko4_dget(int pos) {
    if (pos < 0 || pos >= LIKO4_SAVE_SLOTS) return 0;
    if (!storage_loaded) liko4_save_init(NULL);
    return storage_data[pos];
}

bool liko4_save_all(void) {
    if (!storage_loaded) return false;

#ifdef __EMSCRIPTEN__
    // Web: пишем в localStorage под ключом 'storage'
    int ok = js_localstorage_save(storage_data, (int)sizeof(storage_data));

    if (ok) {
        TraceLog(LOG_INFO, "LIKO-4: Save written to localStorage.");
    } else {
        TraceLog(LOG_WARNING, "LIKO-4: Failed to write localStorage.");
    }
    return ok != 0;

#else
    // Native: файл
    bool ok = SaveFileData(save_path, storage_data, sizeof(storage_data));
    if (ok) {
        TraceLog(LOG_INFO, "LIKO-4: Save written to '%s'.", save_path);
    } else {
        TraceLog(LOG_WARNING, "LIKO-4: Failed to write '%s'.", save_path);
    }
    return ok;
#endif
}