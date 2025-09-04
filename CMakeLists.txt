cmake_minimum_required(VERSION 3.15)
project(ftdi_nuklear_sdl2 C)

set(CMAKE_C_STANDARD 11)

option(USE_STUB "Build with stub FTDI driver" ON)
option(USE_D2XX "Build with FTDI D2XX driver (Windows only)" OFF)

# OpenGL
find_package(OpenGL REQUIRED)

# Nuklear headers (ต้องมีใน third_party/)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# --- Sources ---
set(SRC
    src/main.c
    src/ui.c
    src/ringbuffer.c
    src/worker.c
    src/nk_impl.c       # << ไฟล์ที่ประกาศ NK_IMPLEMENTATION/SDL_GL3_IMPLEMENTATION
)
set(HEADERS
    include/common.h
    include/ui.h
    include/ringbuffer.h
    include/worker.h
    include/ftdi.h
)

if (USE_STUB)
    list(APPEND SRC src/ftdi_stub.c)
endif()

if (USE_D2XX)
    if (WIN32)
        list(APPEND SRC src/ftdi_d2xx.c)
        find_path(D2XX_INCLUDE_DIR ftd2xx.h)
        find_library(D2XX_LIBRARY ftd2xx)
        if (D2XX_INCLUDE_DIR)
            include_directories(${D2XX_INCLUDE_DIR})
        endif()
    else()
        message(FATAL_ERROR "D2XX is Windows-only in this example.")
    endif()
endif()

add_executable(ftdi_nuklear_sdl2 ${SRC} ${HEADERS})
target_include_directories(ftdi_nuklear_sdl2 PRIVATE include)

# --- SDL2 (vcpkg CONFIG) + fallback MODULE ---
find_package(SDL2 QUIET CONFIG)
if (SDL2_FOUND)
    message(STATUS "SDL2 found via CONFIG")
    target_link_libraries(ftdi_nuklear_sdl2
        PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    )
else()
    message(STATUS "SDL2 CONFIG not found; trying MODULE mode")
    find_package(SDL2 REQUIRED)
    target_include_directories(ftdi_nuklear_sdl2 PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ftdi_nuklear_sdl2 PRIVATE ${SDL2_LIBRARIES})
endif()

# --- GLAD (ต้องติดตั้งผ่าน vcpkg ก่อน: vcpkg install glad:x64-windows) ---
find_package(glad CONFIG REQUIRED)
target_link_libraries(ftdi_nuklear_sdl2 PRIVATE glad::glad ${OPENGL_gl_LIBRARY})

# D2XX lib ถ้าเปิดใช้
if (USE_D2XX AND D2XX_LIBRARY)
    target_link_libraries(ftdi_nuklear_sdl2 PRIVATE ${D2XX_LIBRARY})
endif()
