cmake_minimum_required(VERSION 3.15)
project(liko4 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================
# raylib — выбираем по платформе
# ============================================================
if(EMSCRIPTEN)
    set(RAYLIB_DIR "${CMAKE_SOURCE_DIR}/raylib-6.0_webassembly")
elseif(WIN32)
    set(RAYLIB_DIR "${CMAKE_SOURCE_DIR}/raylib-6.0_win64_mingw-w64")
elseif(APPLE)
    set(RAYLIB_DIR "${CMAKE_SOURCE_DIR}/raylib-6.0_macos")
else()
    # Linux
    set(RAYLIB_DIR "${CMAKE_SOURCE_DIR}/raylib-6.0_linux_amd64")
endif()

set(RAYLIB_INCLUDE_DIR "${RAYLIB_DIR}/include")
set(RAYLIB_LIB_DIR     "${RAYLIB_DIR}/lib")

# ============================================================
# Lua 5.4.9 — общий для всех платформ
# ============================================================
set(LUA_DIR "${CMAKE_SOURCE_DIR}/lua")

set(LUA_SRC
    ${LUA_DIR}/lapi.c
    ${LUA_DIR}/lauxlib.c
    ${LUA_DIR}/lbaselib.c
    ${LUA_DIR}/lcode.c
    ${LUA_DIR}/lcorolib.c
    ${LUA_DIR}/lctype.c
    ${LUA_DIR}/ldblib.c
    ${LUA_DIR}/ldebug.c
    ${LUA_DIR}/ldo.c
    ${LUA_DIR}/ldump.c
    ${LUA_DIR}/lfunc.c
    ${LUA_DIR}/lgc.c
    ${LUA_DIR}/linit.c
    ${LUA_DIR}/liolib.c
    ${LUA_DIR}/llex.c
    ${LUA_DIR}/lmathlib.c
    ${LUA_DIR}/lmem.c
    ${LUA_DIR}/loadlib.c
    ${LUA_DIR}/lobject.c
    ${LUA_DIR}/lopcodes.c
    ${LUA_DIR}/loslib.c
    ${LUA_DIR}/lparser.c
    ${LUA_DIR}/lstate.c
    ${LUA_DIR}/lstring.c
    ${LUA_DIR}/lstrlib.c
    ${LUA_DIR}/ltable.c
    ${LUA_DIR}/ltablib.c
    ${LUA_DIR}/ltm.c
    ${LUA_DIR}/lundump.c
    ${LUA_DIR}/lutf8lib.c
    ${LUA_DIR}/lvm.c
    ${LUA_DIR}/lzio.c
)

add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua PUBLIC "${LUA_DIR}")

if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(lua PUBLIC m ${CMAKE_DL_LIBS})
endif()

# ============================================================
# liko4
# ============================================================
set(SOURCES
    src/liko4.c
    src/liko4_api.c
    src/liko4_api_lua.c
    src/liko4_log.c
    src/liko4_sprites.c
    src/liko4_audio.c
    src/liko4_save.c
)

if(NOT EMSCRIPTEN)
    list(APPEND SOURCES
        src/liko4_cart.c
        miniz/miniz.c
    )
endif()

add_executable(liko4 ${SOURCES})

target_include_directories(liko4 PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${RAYLIB_INCLUDE_DIR}"
)

if(NOT EMSCRIPTEN)
    target_include_directories(liko4 PRIVATE
        "${CMAKE_SOURCE_DIR}/miniz"
    )
endif()

# ============================================================
# Emscripten (web) — сборка через emcmake
# ============================================================
if(EMSCRIPTEN)

    target_include_directories(liko4 PRIVATE
        "${RAYLIB_INCLUDE_DIR}"
    )

    target_link_directories(liko4 PRIVATE
        "${RAYLIB_LIB_DIR}"
    )

    # raylib для web называется libraylib.web.a
    target_link_libraries(liko4 PRIVATE raylib.web lua)

    target_link_options(liko4 PRIVATE
        -sUSE_GLFW=3
        -sASYNCIFY
        -sTOTAL_MEMORY=64MB
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=1MB
        "-sEXPORTED_RUNTIME_METHODS=['callMain','FS']"
        "--shell-file=${CMAKE_SOURCE_DIR}/web/shell.html"
        -O2
    )

    set_target_properties(liko4 PROPERTIES
        SUFFIX ".html"
        OUTPUT_NAME "liko4"
        WIN32_EXECUTABLE FALSE
    )

    # ─── Генерация liko4.json, если его нет ───
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/liko4.json")
        file(WRITE "${CMAKE_BINARY_DIR}/liko4.json"
             "{\n  \"cart\": \"game.lk4\"\n}\n")
        message(STATUS "Created default liko4.json")
    endif()

# ============================================================
# Windows / Linux / macOS — обычная сборка
# ============================================================
else()

    # Оптимизация под размер (Release)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
        target_compile_options(liko4 PRIVATE
            -ffunction-sections
            -fdata-sections
        )
    endif()

    # Скрыть окно консоли на Windows (GUI-приложение)
    set_target_properties(liko4 PROPERTIES WIN32_EXECUTABLE TRUE)

    if(WIN32 AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(liko4 PRIVATE
            -mwindows
            -Wl,--gc-sections
            -s
        )
    elseif(APPLE)
        target_link_options(liko4 PRIVATE
            -Wl,-dead_strip
        )
    else()
        target_link_options(liko4 PRIVATE
            -Wl,--gc-sections
            -s
        )
    endif()

    target_include_directories(liko4 PRIVATE
        "${RAYLIB_INCLUDE_DIR}"
    )

    target_link_directories(liko4 PRIVATE "${RAYLIB_LIB_DIR}")

    # Платформенные библиотеки
    if(WIN32)
        target_link_libraries(liko4 PRIVATE raylib lua gdi32 winmm)
    elseif(APPLE)
        find_library(COCOA_LIBRARY Cocoa REQUIRED)
        find_library(IOKIT_LIBRARY IOKit REQUIRED)
        find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
        find_library(OPENGL_LIBRARY OpenGL REQUIRED)
        target_link_libraries(liko4 PRIVATE raylib lua
            ${COCOA_LIBRARY} ${IOKIT_LIBRARY}
            ${COREVIDEO_LIBRARY} ${OPENGL_LIBRARY})
    else()
        find_package(Threads REQUIRED)
        target_link_libraries(liko4 PRIVATE raylib lua
            m ${CMAKE_DL_LIBS} Threads::Threads)
    endif()

    # Копирование data рядом с exe (если есть)
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/data")
        add_custom_command(TARGET liko4 POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/src/data"
                "$<TARGET_FILE_DIR:liko4>/data"
            COMMENT "Copying data to build directory"
        )
    else()
        message(STATUS "src/data/ not found — skipping copy")
    endif()

    # ============================================================
    # Запуск (только для нативной сборки)
    # ============================================================
    add_custom_target(run
        COMMAND $<TARGET_FILE:liko4>
        DEPENDS liko4
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

endif()