#include <limits.h>
#include "ui.h"
#include "common.h"
#include "ringbuffer.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Nuklear
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

static void to_hex_line(const uint8_t* data, size_t n, char* out, size_t cap) {
    size_t pos = 0;
    for (size_t i=0; i<n && pos+3<cap; ++i) {
        pos += (size_t)snprintf(out+pos, cap-pos, "%02X ", data[i]);
    }
    if (pos < cap) out[pos] = 0;
}

void ui_init(app_ui_t* ui, worker_t* w) {
    memset(ui, 0, sizeof(*ui));
    ui->worker = w;
    ui->driver_select = 0; // stub default
    ui->auto_scroll = 1;
}

void ui_cleanup(app_ui_t* ui) {
    (void)ui;
}

void ui_frame(app_ui_t* ui, void* nkctx) {
    struct nk_context* ctx = (struct nk_context*)nkctx;

    if (nk_begin(ctx, "Main", nk_rect(10,10,780,560),
        NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 30, 2);

        // Driver select & Connect/Disconnect
        nk_label(ctx, "Driver:", NK_TEXT_LEFT);
        static const char* drivers[] = { "Stub", "D2XX (Windows)" };
        ui->driver_select = nk_combo(ctx, drivers, 2, ui->driver_select, 25, nk_vec2(220,200));

        nk_layout_row_dynamic(ctx, 30, 3);
        if (nk_button_label(ctx, "Connect")) {
            ftdi_driver_kind_t kind = (ui->driver_select == 1) ? FTDI_DRIVER_D2XX : FTDI_DRIVER_STUB;
            const ftdi_vtbl_t* drv = ftdi_get_driver(kind);
            worker_start(ui->worker, drv);
        }
        if (nk_button_label(ctx, "Disconnect")) {
            worker_stop(ui->worker);
        }
        if (nk_button_label(ctx, "Clear Console")) {
            rb_clear(ui->worker->rxbuf);
        }

        nk_layout_row_dynamic(ctx, 26, 1);
        nk_label(ctx, "Input (send as bytes / raw):", NK_TEXT_LEFT);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, ui->input_line, sizeof(ui->input_line), nk_filter_default);

        nk_layout_row_dynamic(ctx, 28, 2);
        if (nk_button_label(ctx, "Send")) {
            const char* s = ui->input_line;
            if (s[0]) {
                worker_send(ui->worker, (const uint8_t*)s, strlen(s));
                ui->input_line[0] = 0;
            }
        }
        nk_checkbox_label(ctx, "Auto-scroll", &ui->auto_scroll);

        // Console
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Console (RX):", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 300, 1);
        if (nk_group_begin(ctx, "console", NK_WINDOW_BORDER)) {
            uint8_t tmp[512];
            char line[2048];
            size_t got = 0;
            do {
                got = rb_pop(ui->worker->rxbuf, tmp, sizeof(tmp));
                if (got) {
                    to_hex_line(tmp, got, line, sizeof(line));
                    nk_layout_row_dynamic(ctx, 16, 1);
                    nk_label(ctx, line, NK_TEXT_LEFT);
                }
            } while (got);

            if (ui->auto_scroll) {
                nk_group_set_scroll(ctx, "console", INT_MAX, INT_MAX);
            }
            nk_group_end(ctx);
        }

        // Stats
        nk_layout_row_dynamic(ctx, 22, 1);
        char stat[256];
        snprintf(stat, sizeof(stat), "RX: %llu bytes   TX: %llu bytes", 
            (unsigned long long)ui->worker->bytes_rx, (unsigned long long)ui->worker->bytes_tx);
        nk_label(ctx, stat, NK_TEXT_LEFT);
    }
    nk_end(ctx);
}
