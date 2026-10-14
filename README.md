# LIKO-4

A tiny fantasy console for Lua. Built for making small 2D games fast.

- 128×128 virtual screen, 4× window (512×512)
- 8-color palette (Pico micro) with runtime palette swapping
- 256 sprites (16×16 grid of 8×8 tiles)
- Sprites from indexed PNG, transparency via magenta `#FF00FF`
- Sound and music loaded from files
- Tone synthesis — 4 channels (WASM-4 style)
- Mouse and keyboard input
- Save system — 64 slots × 8 bytes
- Cartridges — single `.lk4` file (ZIP with `data/` inside)
- 65536-character script limit — all `.lua` files in `scripts/` combined
- Lua scripting with `require`
- Single ~1 MB executable, no external DLLs
- Windows, Linux, macOS and Web builds

## The main idea  
LIKO-4 is the materialization of a fantasy console that never existed but could have: drawing inspiration from PICO-4 — an imaginary younger model in the PICO lineup, conceived and drawn by fans long before the runtime appeared.

The name LIKO-4 is inspired by Rami-Sabbagh's [LIKO-12](https://github.com/LIKO-12/LIKO-12?ysclid=mv6nght88j992711274), as part of a series of open-source consoles.

Unlike LIKO-12, LIKO-4 is not written in LOVE2D; it is written from scratch using raylib.

---

## Font

Default font: `data/sprites/font.png` — a 128×128 sheet with 3×5 glyphs
(inspired by PICO-8). Replace it with your own if you want.

---

## Building

### Requirements

- **CMake 3.15+**
- **Compiler:** MinGW-w64 (Windows) / GCC (Linux) / Clang (macOS)
- **raylib 6.0** — download from [raylib releases](https://github.com/raysan5/raylib/releases)

### raylib folders

Place the matching raylib release in the project root:

| Platform | Folder | Archive |
|---|---|---|
| **Windows** | `raylib-6.0_win64_mingw-w64/` | `raylib-6.0_win64_mingw-w64.zip` |
| **Linux** | `raylib-6.0_linux_amd64/` | `raylib-6.0_linux_amd64.tar.gz` |
| **macOS** | `raylib-6.0_macos/` | `raylib-6.0_macos.tar.gz` |
| **Web** | `raylib-6.0_webassembly/` | `raylib-6.0_webassembly.zip` |

The Windows version is included in this repository.
Others must be downloaded separately.

### Desktop build (Windows / Linux / macOS)

```bash
cmake -B build
cmake --build build
```

Output:

- **Windows:** `build/liko4.exe`
- **Linux / macOS:** `build/liko4`

### Web build (Emscripten)

Requires [Emscripten SDK](https://emscripten.org).

```bash
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -B build-web
cmake --build build-web
```

Output: `build-web/liko4.html`, `.js`, `.wasm`.

Then place `liko4.json` and your cart next to `liko4.html`, and serve over HTTP:

```bash
cd build-web
python -m http.server 8000
```

Open `http://localhost:8000/liko4.html`.

---

## Cartridges

A cartridge is a single `.lk4` file — a ZIP archive containing a `data/` folder:

```
game.lk4
└── data/
    ├── scripts/
    ├── sprites/
    ├── sounds/
    └── music/
```

### How the engine finds the game

When you run `liko4` without arguments, it searches in this order:

1. **`game.lk4`** next to the executable → extract to `_cart/`, run from there.
2. **`data/`** next to the executable → run in dev mode (no extraction).
3. **Nothing found** → engine exits with an error in the log.

You can also pass a cart explicitly:

```bash
liko4.exe mygame.lk4
liko4.exe other.lk4
```

This lets you ship one `liko4.exe` with multiple carts.

### Dev mode vs Cart mode

- **Dev mode** — run with a bare `data/` folder next to the exe. Edit `data/scripts/main.lua`, press **Ctrl+R** to reload. No repacking needed.
- **Cart mode** — run with a `.lk4` file. The engine extracts it to `_cart/`, runs, and cleans up on exit.

### Packing a cart

On Windows: right-click `src/data/` → *Send to → Compressed (zipped) folder*, rename to `game.lk4`.

On Linux/macOS:

```bash
cd src
zip -r ../game.lk4 data/
```

### Web

For web builds, the cart is fetched by JavaScript and unpacked into virtual memory. Place next to `liko4.html`:

```
build-web/
├── liko4.html
├── liko4.js
├── liko4.wasm
├── liko4.json     ← { "cart": "game.lk4" }
└── game.lk4       ← the same cart
```

`liko4.json`:

```json
{
  "cart": "game.lk4"
}
```

---

## Running

**Ctrl+R** — reloads the game (Lua, sprites, font, sounds) without restarting the executable.

**F11** — toggle fullscreen.

---

## Architecture

```
liko4.exe
data/
├── scripts/
│   └── main.lua          ← game entry point
├── sprites/
│   ├── font.png          ← 128×128 font sheet (3×5 glyphs)
│   └── spritesheet.png   ← 128×128 sprites (8×8 each, 16×16 grid = 256 sprites)
├── sounds/
│   ├── 0.wav             ← sfx(0)
│   ├── 1.wav             ← sfx(1)
│   └── ...               ← up to 31.wav (32 sounds total)
└── music/
    ├── 0.ogg             ← music(0)
    └── ...               ← up to 3.ogg (4 tracks total)
```

### `data/scripts/`

Lua game code. Entry point is always `main.lua`.

You can split your game into multiple files and use `require`:

```lua
-- main.lua
local player = require("player")
```

### `data/sprites/`

- **`font.png`** — 128×128 font sheet, glyphs 3×5 px.
- **`spritesheet.png`** — 128×128 spritesheet. Each sprite is 8×8 px.
  - Grid: 16×16 = **256 sprites** total.
  - Sprite index `n` → column `n % 16`, row `n / 16`.
  - Transparent pixels: use magenta `#FF00FF`.
  - Colors: use the 8 palette colors only.

### `data/sounds/`

SFX files named `0.wav`, `1.wav`, ... `31.wav`. Loaded automatically at startup.

```lua
sfx(0)         -- play sound 0.wav at full volume
sfx(0, 0.5)    -- play at 50% volume
```

### `data/music/`

Music files named `0.ogg`, `1.ogg`, ... `3.ogg`. Loaded automatically.

```lua
music(0)         -- play 0.ogg in loop
music(0, 0.3)    -- play at 30% volume
music()          -- stop
```

---

## Lua API

**All API functions are global** — call them directly, without a table prefix.

### Game callbacks

```lua
function _init()     -- called once at startup (and after Ctrl+R reload)
end

function _update()   -- logic, called 60 times per second
end

function _draw()     -- rendering, called 60 times per second
end
```

All three are optional.

### Graphics

```lua
cls(color)                          -- clear screen
print(text, x, y, color)            -- draw text
fps(x, y, color)                    -- draw FPS counter
```

### Primitives

```lua
circ(x, y, r, color)                -- filled circle
circb(x, y, r, color)               -- circle border
elli(x, y, a, b, color)             -- filled ellipse
ellib(x, y, a, b, color)            -- ellipse border
rect(x, y, w, h, color)             -- filled rectangle
rectb(x, y, w, h, color)            -- rectangle border
line(x0, y0, x1, y1, color)         -- line
```

### Sprites

```lua
spr(n, x, y)                        -- draw 8×8 sprite n at (x, y)
spr(n, x, y, cols, rows)            -- draw a block of cols×rows sprites
spr(n, x, y, cols, rows, flip_x, flip_y)
```

`cols` and `rows` — how many 8×8 sprites wide and tall to draw. Default 1×1.

Example: `spr(0, 10, 10, 2, 3)` draws a 16×24 block (2 sprites wide, 3 tall) starting from sprite 0.

### Palette

```lua
pal()             -- reset to default
pal(c0, c1)       -- everything drawn with color c0 now draws as c1
```

### Input — keyboard

```lua
btn(id)    -- is button held
btnp(id)   -- was button just pressed this frame
```

Button IDs:

| ID | Button |
|----|--------|
| 0  | LEFT   |
| 1  | RIGHT  |
| 2  | UP     |
| 3  | DOWN   |
| 4  | O (Z / J) |
| 5  | X (X / K) |

Keyboard mapping: arrows / WASD for direction, Z / J for `O`, X / K for `X`.

### Input — mouse

```lua
local m = mouse()   -- current state (buttons held)
local m = mousep()  -- state at this frame (buttons just pressed)
```

Both return a table:

| Field | Type | Meaning |
|-------|------|---------|
| `m.x` | integer | cursor X in virtual screen coords (0–127) |
| `m.y` | integer | cursor Y in virtual screen coords (0–127) |
| `m.left` | bool | left button |
| `m.middle` | bool | middle button |
| `m.right` | bool | right button |
| `m.scrollx` | integer | horizontal scroll delta (−31..32) |
| `m.scrolly` | integer | vertical scroll delta (−31..32) |

Example:

```lua
function _draw()
    cls(0)
    local m = mouse()
    circ(m.x, m.y, 2, 3)
    if m.left then rectb(m.x - 4, m.y - 4, 8, 8, 1) end
end
```

### Time

```lua
utime()    -- Unix timestamp in seconds (integer)
time()     -- milliseconds elapsed since game start (float)
```

- `utime()` — for idle games, persistent progress, saving timestamps.
- `time()` — for animations, timers, blinking.

```lua
-- animation
local elapsed = time() / 1000
circ(80 + math.sin(elapsed * 2) * 30, 80, 5, 3)

-- idle
local last = dget(0)
local now = utime()
coins = coins + (now - last) * 0.1
dset(0, now)
```

### Sound

```lua
sfx(n)             -- play sound n
sfx(n, volume)     -- play with volume 0.0–1.0
music(n)           -- play music n in loop
music(n, volume)   -- play with volume
music()            -- stop current music
```

### Tone synthesis

Procedural sound generation — like WASM-4. Useful for small carts,
no WAV files needed.

```lua
tone(frequency, duration, volume, flags)
```

- `frequency` — Hz (e.g. 440, 880)
- `duration` — in frames (60 frames = 1 second)
- `volume` — 0 to 100
- `flags` — channel / waveform (0–3)

4 channels, one sound per channel. A new tone on the same channel
replaces the previous one.

| flags | Channel | Waveform |
|-------|---------|----------|
| 0 | 0 | pulse 12.5% |
| 1 | 1 | pulse 25% |
| 2 | 2 | triangle |
| 3 | 3 | noise |

Examples:

```lua
-- Jump — pulse 12.5%
tone(660, 6, 80, 0)

-- Shoot — pulse 25%
tone(880, 4, 70, 1)

-- Bassy note — triangle
tone(220, 30, 40, 2)

-- Explosion — noise
tone(0, 15, 90, 3)

-- All 4 can play at the same time
```

### Save

```lua
dset(pos, value)   -- write value (int64) to slot pos (0–63)
dget(pos)          -- read value from slot pos
```

- On desktop: auto-saved to `save/storage.bin` on window close and every 10 minutes.
- On web: saved to browser `localStorage` immediately on every `dset()`.

---

## Lua modules

LIKO-4 comes with two optional Lua modules:

### `camera.lua` — scrolling camera

Adds `camera(x, y)` to the API. When called, all subsequent
drawing functions are offset by `(-x, -y)`. Call `camera()`
to reset.

```lua
require("camera")

function _draw()
    cls(3)
    camera(player.x - 76, player.y - 76)
    spr(0, player.x, player.y)
    camera()   -- reset for HUD
    print("HP", 1, 1, 0)
end
```

### `map.lua` — tilemap drawing

Adds `map()`, `mget()`, `mset()` to the API.
Draws a flat tilemap loaded from Tiled.

```lua
require("map")

function _init()
    map_data = { 2, 2, 0, 0, 2, 2, ... }
    map_w = 128
    map_h = 128
end

function _draw()
    map(0, 0, 0, 0, 20, 20)
end
```

Fields:
- `map_data` — flat array from Tiled (1-based GIDs, 0 = empty)
- `map_w`, `map_h` — map dimensions in tiles (defaults 128×128)

Functions:
- `map(celx, cely, sx, sy, celw, celh)` — draw a region
- `mget(x, y)` — read tile (0-based sprite id, 0 = empty)
- `mset(x, y, id)` — write tile

Convention: **sprite 0 is reserved** — don't use it in maps.

---

## Example

`data/scripts/main.lua`:

```lua
local x, y

function _init()
    x = 80
    y = 80
end

function _update()
    if btn(0) then x = x - 1 end
    if btn(1) then x = x + 1 end
    if btn(2) then y = y - 1 end
    if btn(3) then y = y + 1 end

    if btnp(4) then sfx(0) end
end

function _draw()
    cls(0)
    spr(0, x - 4, y - 4)
    print("HELLO", 4, 4, 3)
end
```

---

## Credits

- [raylib](https://raylib.com) — zlib license
- [Lua 5.4.9](https://lua.org) — MIT license
- [Pico micro Palette](https://lospec.com/palette-list/pico-micro) by Space Sandwich — 8 colors

---

## License

MIT — see `LICENSE`.