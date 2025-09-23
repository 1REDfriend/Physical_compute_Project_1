#include "common.h"
#include "ui.h"
#include "worker.h"
#include "obd_parser.h"
#include "esp32_driver.h"
#include "obd_commands.h"
#include "dashboard.h"
#include "dtc_viewer.h"
#include "data_logger.h"
#include "connection_manager.h"
#include "data_export.h"
#include "settings.h"

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <stdio.h>

// Ensure consistent public API macros across TUs (no implementation here)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window *win = SDL_CreateWindow(APP_NAME,
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!win)
    {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glctx = SDL_GL_CreateContext(win);
    if (!glctx)
    {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    if (SDL_GL_SetSwapInterval(1) < 0)
    {
        fprintf(stderr, "Warning: Unable to set VSync: %s\n", SDL_GetError());
    }


    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        fprintf(stderr, "Failed to load OpenGL via GLAD\n");
        SDL_GL_DeleteContext(glctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }


    // Initialize Nuklear UI
    struct nk_context *ctx = nk_sdl_init(win);
    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();

    worker_t worker = {0};
    app_ui_t ui = {0};
    ui_init(&ui, &worker);

    // Initialize OBD-II components
    obd_parser_t *obd_parser = obd_parser_create(1024 * 1024); // 1MB buffer
    esp32_driver_t *esp32_driver = esp32_driver_create();
    obd_command_library_t *obd_commands = obd_command_library_create();
    dashboard_state_t *dashboard = dashboard_create();
    dtc_viewer_t *dtc_viewer = dtc_viewer_create();
    data_logger_t *data_logger = data_logger_create();
    connection_manager_t *connection_manager = connection_manager_create();
    export_manager_t *export_manager = export_manager_create();
    settings_manager_t *settings_manager = settings_manager_create();

    // Initialize components
    if (obd_parser)
        obd_parser_init(obd_parser);
    if (esp32_driver)
        esp32_driver_init(esp32_driver);
    if (obd_commands)
        obd_command_library_init(obd_commands, VEHICLE_MAKE_GENERIC);
    if (dashboard)
        dashboard_init(dashboard, NULL);
    if (dtc_viewer)
        dtc_viewer_init(dtc_viewer);
    if (data_logger)
        data_logger_init(data_logger, NULL);
    if (connection_manager)
        connection_manager_init(connection_manager, 10);
    if (export_manager)
        export_manager_init(export_manager, NULL);
    if (settings_manager)
        settings_manager_init(settings_manager, "./config.ini");

    int running = 1;
    while (running && !ui.want_quit)
    {

        SDL_Event evt;
        (void)evt; // Suppress unused variable warning
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_QUIT)
                running = 0;
            nk_sdl_handle_event(&evt);
        }
        nk_input_end(ctx);

        // UI frame
        ui_frame(&ui, ctx);

        // Render
        int win_w, win_h;
        SDL_GetWindowSize(win, &win_w, &win_h);
        glViewport(0, 0, win_w, win_h);
        glClear(GL_COLOR_BUFFER_BIT);
        nk_sdl_render(NK_ANTI_ALIASING_ON, 512 * 1024, 128 * 1024);
        SDL_GL_SwapWindow(win);
    }

    worker_stop(&worker);

    // Cleanup OBD-II components
    if (obd_parser)
        obd_parser_destroy(obd_parser);
    if (esp32_driver)
        esp32_driver_destroy(esp32_driver);
    if (obd_commands)
        obd_command_library_destroy(obd_commands);
    if (dashboard)
        dashboard_destroy(dashboard);
    if (dtc_viewer)
        dtc_viewer_destroy(dtc_viewer);
    if (data_logger)
        data_logger_destroy(data_logger);
    if (connection_manager)
        connection_manager_destroy(connection_manager);
    if (export_manager)
        export_manager_destroy(export_manager);
    if (settings_manager)
        settings_manager_destroy(settings_manager);

    nk_sdl_shutdown();
    SDL_Quit();
    return 0;
}
