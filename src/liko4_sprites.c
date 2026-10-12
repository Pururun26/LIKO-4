#include "liko4_sprites.h"
#include "liko4_conf.h"
#include "liko4_api.h"
#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>

// ============================================================
// Состояние
// ============================================================

static uint8_t  *sprite_indices = NULL;   // RGBA → индексы палитры (0–7, 255 = прозрачный)
static Texture2D sprite_sheet   = {0};    // текстура, где R=G=B=индекс, A=прозрачность
static bool      loaded         = false;

// Шейдер палитры: берёт индекс из текстуры и подставляет цвет из uniform.
static Shader palette_shader = {0};
static int    palette_locs[LIKO4_COLOR_COUNT];  // location uniform "pal0".."pal7"
static bool   shader_ready   = false;
static bool   palette_dirty  = true;      // нужно ли обновить uniform палитры

// ============================================================
// Исходник шейдера
//   Web (WebGL1/GLSL ES 1.00) требует precision.
//   Desktop (GLSL 330) — не требует, и с ним ломается.
//   Raylib сам подставит #version.
// ============================================================

#ifdef __EMSCRIPTEN__
static const char *PALETTE_FS =
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform vec4 pal0;\n"
    "uniform vec4 pal1;\n"
    "uniform vec4 pal2;\n"
    "uniform vec4 pal3;\n"
    "uniform vec4 pal4;\n"
    "uniform vec4 pal5;\n"
    "uniform vec4 pal6;\n"
    "uniform vec4 pal7;\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture2D(texture0, fragTexCoord);\n"
    "    if (texel.a < 0.5) discard;\n"
    "    float idx = texel.r * 255.0;\n"
    "    vec4 col;\n"
    "    if (idx < 0.5) col = pal0;\n"
    "    else if (idx < 1.5) col = pal1;\n"
    "    else if (idx < 2.5) col = pal2;\n"
    "    else if (idx < 3.5) col = pal3;\n"
    "    else if (idx < 4.5) col = pal4;\n"
    "    else if (idx < 5.5) col = pal5;\n"
    "    else if (idx < 6.5) col = pal6;\n"
    "    else col = pal7;\n"
    "    gl_FragColor = col * colDiffuse;\n"
    "}\n";
#else
static const char *PALETTE_FS =
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform vec4 pal0;\n"
    "uniform vec4 pal1;\n"
    "uniform vec4 pal2;\n"
    "uniform vec4 pal3;\n"
    "uniform vec4 pal4;\n"
    "uniform vec4 pal5;\n"
    "uniform vec4 pal6;\n"
    "uniform vec4 pal7;\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture2D(texture0, fragTexCoord);\n"
    "    if (texel.a < 0.5) discard;\n"
    "    float idx = texel.r * 255.0;\n"
    "    vec4 col;\n"
    "    if (idx < 0.5) col = pal0;\n"
    "    else if (idx < 1.5) col = pal1;\n"
    "    else if (idx < 2.5) col = pal2;\n"
    "    else if (idx < 3.5) col = pal3;\n"
    "    else if (idx < 4.5) col = pal4;\n"
    "    else if (idx < 5.5) col = pal5;\n"
    "    else if (idx < 6.5) col = pal6;\n"
    "    else col = pal7;\n"
    "    gl_FragColor = col * colDiffuse;\n"
    "}\n";
#endif

// ============================================================
// Конвертация цвета в индекс палитры
// ============================================================

// Возвращает ближайший индекс палитры для цвета c.
// Прозрачные пиксели → 255 (спец-значение «не рисовать»).
static uint8_t nearest_palette_index(Color c) {
    // прозрачные пиксели (альфа = 0)
    if (c.a == 0) return 255;

    // маджента (#FF00FF) — ключ прозрачности
    if (c.r == 255 && c.g == 0 && c.b == 255) return 255;

    // иначе — ближайший цвет палитры
    int best = 0;
    int best_dist = 1 << 30;
    for (int i = 0; i < LIKO4_COLOR_COUNT; i++) {
        Color p = liko4_get_palette(i);
        int dr = (int)c.r - p.r;
        int dg = (int)c.g - p.g;
        int db = (int)c.b - p.b;
        int dist = dr*dr + dg*dg + db*db;
        if (dist < best_dist) {
            best_dist = dist;
            best = i;
        }
    }
    return (uint8_t)best;
}

// ============================================================
// Uniform палитры — заливаем цвета в шейдер
// ============================================================

static void update_palette_uniform(void) {
    if (!shader_ready || !palette_dirty) return;

    // LIKO4_COLOR_COUNT цветов по 4 компонента (RGBA)
    float data[LIKO4_COLOR_COUNT * 4];
    for (int i = 0; i < LIKO4_COLOR_COUNT; i++) {
        Color c = liko4_get_palette(i);
        data[i*4 + 0] = c.r / 255.0f;
        data[i*4 + 1] = c.g / 255.0f;
        data[i*4 + 2] = c.b / 255.0f;
        data[i*4 + 3] = c.a / 255.0f;
    }

    for (int i = 0; i < LIKO4_COLOR_COUNT; i++) {
        if (palette_locs[i] >= 0) {
            SetShaderValue(palette_shader, palette_locs[i],
                        &data[i * 4], SHADER_UNIFORM_VEC4);
        }
    }
    palette_dirty = false;
}

// ============================================================
// Загрузка
// ============================================================

void liko4_load_spritesheet(const char *filepath) {
    Image img = LoadImage(filepath);

    if (img.data == NULL) {
        // Заглушка: пустой лист 256×256, все пиксели прозрачные.
        img = GenImageColor(LIKO4_SPRITE_SHEET_W,
                            LIKO4_SPRITE_SHEET_H, BLANK);
        TraceLog(LOG_WARNING,
                 "LIKO-4: Spritesheet '%s' not found. Using blank.", filepath);
    }

    if (img.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }

    int count = img.width * img.height;

    // Освобождаем прошлый буфер, если был
    if (sprite_indices) free(sprite_indices);
    sprite_indices = malloc(count);
    if (!sprite_indices) {
        UnloadImage(img);
        return;
    }

    // RGBA → индексы палитры
    Color *px = (Color *)img.data;
    for (int i = 0; i < count; i++) {
        sprite_indices[i] = nearest_palette_index(px[i]);
    }

    // Готовим ИНДЕКСНУЮ текстуру: R=G=B=индекс, A=0 для прозрачного.
    // Шейдер потом превратит эти индексы в цвета палитры.
    Color *indexed = malloc(count * sizeof(Color));
    if (!indexed) {
        UnloadImage(img);
        return;
    }
    for (int i = 0; i < count; i++) {
        uint8_t idx = sprite_indices[i];
        if (idx == 255) {
            indexed[i] = (Color){ 0, 0, 0, 0 };      // прозрачный
        } else {
            indexed[i] = (Color){ idx, idx, idx, 255 };
        }
    }

    Image idx_img = {
        .data    = indexed,
        .width   = img.width,
        .height  = img.height,
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    UnloadImage(img);

    // Создаём текстуру из индексов
    if (sprite_sheet.id != 0) UnloadTexture(sprite_sheet);
    sprite_sheet = LoadTextureFromImage(idx_img);
    SetTextureFilter(sprite_sheet, TEXTURE_FILTER_POINT);

    int sheet_w = idx_img.width;
    int sheet_h = idx_img.height;

    free(indexed);

    // Загружаем шейдер палитры — один раз
    if (!shader_ready) {
        palette_shader = LoadShaderFromMemory(0, PALETTE_FS);
        if (palette_shader.id == 0) {
            TraceLog(LOG_ERROR, "LIKO-4: Palette shader failed to load!");
        } else {
            for (int i = 0; i < LIKO4_COLOR_COUNT; i++) {
                char name[8];
                snprintf(name, sizeof(name), "pal%d", i);
                palette_locs[i] = GetShaderLocation(palette_shader, name);
            }

            bool all_found = true;
            for (int i = 0; i < LIKO4_COLOR_COUNT; i++) {
                TraceLog(LOG_INFO, "LIKO-4: pal%d loc=%d", i, palette_locs[i]);
                if (palette_locs[i] < 0) all_found = false;
            }
            if (!all_found) {
                TraceLog(LOG_WARNING, "LIKO-4: Some palette uniforms not found!");
            }
            shader_ready = true;
        }
    }

    palette_dirty = true;
    loaded        = true;

    TraceLog(LOG_INFO, "LIKO-4: Spritesheet '%s' loaded (%dx%d).",
             filepath, sheet_w, sheet_h);
}

// ============================================================
// Rebuild — теперь просто помечаем палитру «грязной».
// Реальное обновление произойдёт перед первым spr за кадр.
// ============================================================

void liko4_rebuild_spritesheet(void) {
    palette_dirty = true;
}

// ============================================================
// Рисование
// ============================================================

void spr_pro(int16_t n, int16_t x, int16_t y,
             uint8_t cols, uint8_t rows,
             bool flip_x, bool flip_y) {
    if (!loaded) return;
    if (cols == 0 || rows == 0) return;
    if (n < 0) return;

    int16_t col = n % LIKO4_SPRITE_SHEET_TILES;
    int16_t row = n / LIKO4_SPRITE_SHEET_TILES;

    int16_t pixel_w = cols * LIKO4_SPRITE_SIZE;
    int16_t pixel_h = rows * LIKO4_SPRITE_SIZE;

    Rectangle src = {
        (float)(col * LIKO4_SPRITE_SIZE),
        (float)(row * LIKO4_SPRITE_SIZE),
        (float)pixel_w,
        (float)pixel_h
    };

    if (flip_x) src.width  = -(float)pixel_w;
    if (flip_y) src.height = -(float)pixel_h;

    Rectangle dest = { (float)x, (float)y, (float)pixel_w, (float)pixel_h };

    if (shader_ready) {
        update_palette_uniform();
        BeginShaderMode(palette_shader);
            DrawTexturePro(sprite_sheet, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
        EndShaderMode();
    } else {
        // Fallback — без палитры, увидим серые оттенки индексов
        DrawTexturePro(sprite_sheet, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
    }
}

void spr(int16_t n, int16_t x, int16_t y) {
    spr_pro(n, x, y, 1, 1, false, false);
}