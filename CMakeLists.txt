cmake_minimum_required(VERSION 3.15)
# ถ้าจะใส่ toolchain ในไฟล์ ต้องใส่ก่อน project() เสมอ
# set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

project(obd_reader C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(USE_STUB "Build with stub FTDI driver" ON)
option(USE_D2XX "Build with FTDI D2XX driver (Windows only)" OFF)
option(USE_SDL_STUB "Build with SDL stub" ON)  # ตั้งจาก CLI ให้เป็น OFF เมื่อใช้ SDL

# === Packages ===
if(NOT USE_SDL_STUB)
    find_package(SDL2 CONFIG REQUIRED)          # vcpkg -> SDL2::SDL2 และ SDL2::SDL2main
    find_package(glad CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
else()
    message(STATUS "Using SDL stub - no external dependencies")
endif()
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# === Sources ===
set(SRC
    src/main.c
    src/ui.c
    src/ringbuffer.c
    src/worker.c
    src/ftdi_usb_enum.c
    src/ftdi_stub.c
    src/obd_parser.c
    src/esp32_driver.c
    src/esp32_driver_init.c
    src/obd_commands.c
    src/dashboard.c
    src/dtc_viewer.c
    src/data_logger.c
    src/connection_manager.c
    src/data_export.c
    src/settings.c
)

# ไฟล์ที่พึ่งพา SDL/OpenGL ใส่เฉพาะตอนใช้ SDL จริง
if(NOT USE_SDL_STUB)
    list(APPEND SRC src/nk_impl.c)   # มี NK_IMPLEMENTATION/SDL_GL3_IMPLEMENTATION
endif()

set(HEADERS
    include/common.h
    include/ui.h
    include/ringbuffer.h
    include/worker.h
    include/ftdi.h
    include/obd_parser.h
    include/esp32_driver.h
    include/obd_commands.h
    include/dashboard.h
    include/dtc_viewer.h
    include/data_logger.h
    include/connection_manager.h
    include/data_export.h
    include/settings.h
)

add_executable(obd_reader ${SRC} ${HEADERS})
target_include_directories(obd_reader PRIVATE include)

if(NOT USE_SDL_STUB)
    # บน MinGW มักต้องลิงก์ SDL2main ด้วย
    target_compile_definitions(obd_reader PRIVATE SDL_MAIN_HANDLED)
    target_link_libraries(obd_reader PRIVATE SDL2::SDL2 glad::glad OpenGL::GL)
else()
    target_compile_definitions(obd_reader PRIVATE NO_SDL2)
endif()

if(WIN32)
    target_link_libraries(obd_reader PRIVATE setupapi)
else()
    target_link_libraries(obd_reader PRIVATE pthread)
endif()

if(MSVC)
    target_compile_options(obd_reader PRIVATE /W4)
    target_compile_definitions(obd_reader PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(obd_reader PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS obd_reader DESTINATION bin)
