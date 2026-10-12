#include "liko4_cart.h"
#include "raylib.h"
#include "miniz.h"

#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

// ============================================================
// Проверка расширения .lk4
// ============================================================

bool liko4_cart_is_cart(const char *path) {
    if (!path) return false;
    size_t len = strlen(path);
    if (len < 3) return false;
    return strcmp(path + len - 3, ".lk4") == 0;
}

// ============================================================
// Создать все родительские папки для пути
// ============================================================

static void make_parent_dirs(const char *full_path, const char *base) {
    size_t base_len = strlen(base);

    char buf[1024];
    strncpy(buf, full_path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    for (size_t i = base_len + 1; buf[i]; i++) {
        if (buf[i] == '/' || buf[i] == '\\') {
            char saved = buf[i];
            buf[i] = '\0';

            if (!DirectoryExists(buf)) {
                MakeDirectory(buf);
            }

            buf[i] = saved;
        }
    }
}

// ============================================================
// Рекурсивное удаление папки
// ============================================================

void liko4_cart_remove(const char *dir) {
    if (!dir) return;
    if (!DirectoryExists(dir)) return;

    FilePathList entries = LoadDirectoryFiles(dir);

    for (unsigned i = 0; i < entries.count; i++) {
        if (DirectoryExists(entries.paths[i])) {
            liko4_cart_remove(entries.paths[i]);
        } else {
            FileRemove(entries.paths[i]);
        }
    }

    UnloadDirectoryFiles(entries);

#ifdef _WIN32
    _rmdir(dir);
#else
    rmdir(dir);
#endif
}

// ============================================================
// Распаковка .lk4
// ============================================================

bool liko4_cart_extract(const char *cart_path, const char *out_dir) {
    mz_zip_archive zip = {0};

    if (!mz_zip_reader_init_file(&zip, cart_path, 0)) {
        TraceLog(LOG_ERROR, "LIKO-4: cannot open cart '%s'", cart_path);
        return false;
    }

    if (!DirectoryExists(out_dir)) {
        MakeDirectory(out_dir);
    }

    mz_uint count = mz_zip_reader_get_num_files(&zip);
    int extracted = 0;
    int failed    = 0;

    for (mz_uint i = 0; i < count; i++) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, i, &stat)) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s",
                 out_dir, stat.m_filename);

        if (stat.m_is_directory) {
            if (!DirectoryExists(full_path)) {
                MakeDirectory(full_path);
            }
            continue;
        }

        make_parent_dirs(full_path, out_dir);

        if (mz_zip_reader_extract_to_file(&zip, i, full_path, 0)) {
            extracted++;
        } else {
            TraceLog(LOG_WARNING, "LIKO-4: failed to extract '%s'",
                     stat.m_filename);
            failed++;
        }
    }

    mz_zip_reader_end(&zip);

    TraceLog(LOG_INFO, "LIKO-4: cart '%s' → %d files (%d failed)",
             cart_path, extracted, failed);

    return (failed == 0);
}