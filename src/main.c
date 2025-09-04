#include "common.h"
#include "ui.h"
#include "worker.h"

#include <SDL.h>
#include <glad/glad.h>
#include <stdio.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* win = SDL_CreateWindow(APP_NAME,
                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                        800, 600, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glctx = SDL_GL_CreateContext(win);
    SDL_GL_SetSwapInterval(1);

    // โหลดฟังก์ชัน OpenGL ผ่าน GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "Failed to load OpenGL via GLAD\n");
        return 1;
    }

    struct nk_context* ctx = nk_sdl_init(win);
    struct nk_font_atlas* atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();
    
    worker_t worker = {0};
    app_ui_t ui = {0};
    ui_init(&ui, &worker);

    int running = 1;
    while (running && !ui.want_quit) {
        SDL_Event evt;
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) running = 0;
            nk_sdl_handle_event(&evt);
        }
        nk_input_end(ctx);

        // UI frame
        ui_frame(&ui, ctx);

        // Render
        int win_w, win_h;
        SDL_GetWindowSize(win, &win_w, &win_h);
        glViewport(0,0,win_w,win_h);
        glClear(GL_COLOR_BUFFER_BIT);
        nk_sdl_render(NK_ANTI_ALIASING_ON, 512*1024, 128*1024);
        SDL_GL_SwapWindow(win);
    }

    worker_stop(&worker);
    nk_sdl_shutdown();
    SDL_GL_DeleteContext(glctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
