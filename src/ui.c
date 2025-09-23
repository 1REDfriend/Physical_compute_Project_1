#include <limits.h>
#include "ui.h"
#include "ringbuffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Nuklear (public API macros only)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_sdl_gl3.h"

static void to_hex_line(const uint8_t *data, size_t n, char *out, size_t cap)
{
    size_t pos = 0;
    for (size_t i = 0; i < n && pos + 3 < cap; ++i)
    {
        pos += (size_t)snprintf(out + pos, cap - pos, "%02X ", data[i]);
    }
    if (pos < cap)
        out[pos] = 0;
}

void ui_init(app_ui_t *ui, worker_t *w)
{
    memset(ui, 0, sizeof(*ui));
    ui->worker = w;
    ui->current_page = UI_PAGE_OBD_VIEWER; // เริ่มต้นที่หน้า OBD Viewer
    ui->driver_select = 0; // stub default
    ui->auto_scroll = 1;
    ui->selected_reader_device = 0;
    ui->selected_writer_device = 0;
    ui->reader_connected = 0;
    ui->writer_connected = 0;
}

// Navigation menu
static void ui_navigation_menu(app_ui_t *ui, struct nk_context *ctx)
{
    if (nk_begin(ctx, "Navigation", nk_rect(10, 10, 780, 50),
                 NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic(ctx, 30, 3);
        
        if (nk_button_label(ctx, "OBD-II Viewer"))
        {
            ui->current_page = UI_PAGE_OBD_VIEWER;
        }
        if (nk_button_label(ctx, "USB Reader"))
        {
            ui->current_page = UI_PAGE_USB_READER;
        }
        if (nk_button_label(ctx, "USB Writer"))
        {
            ui->current_page = UI_PAGE_USB_WRITER;
        }
    }
    nk_end(ctx);
}

// OBD-II Data Viewer Page
static void ui_obd_viewer_page(app_ui_t *ui, struct nk_context *ctx)
{
    if (nk_begin(ctx, "OBD-II Data Viewer", nk_rect(10, 70, 780, 500),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 30, 2);

        // Driver select & Connect/Disconnect
        nk_label(ctx, "Driver:", NK_TEXT_LEFT);
        static const char *drivers[] = {"Stub", "D2XX (Windows)"};
        ui->driver_select = nk_combo(ctx, drivers, 2, ui->driver_select, 25, nk_vec2(220, 200));

        nk_layout_row_dynamic(ctx, 30, 4);
        if (nk_button_label(ctx, "Connect"))
        {
            ftdi_driver_kind_t kind = (ui->driver_select == 1) ? FTDI_DRIVER_D2XX : FTDI_DRIVER_STUB;
            const ftdi_vtbl_t *drv = ftdi_get_driver(kind);
            worker_start(ui->worker, drv);
        }
        if (nk_button_label(ctx, "Disconnect"))
        {
            worker_stop(ui->worker);
        }
        if (nk_button_label(ctx, "Clear Console"))
        {
            rb_clear(ui->worker->rxbuf);
        }
        if (nk_button_label(ctx, "Refresh"))
        {
            size_t out_count = 0;
            if (FTDI_OK == ftdi_enumerate_devices(ui->devlist, (size_t)32, &out_count))
            {
                ui->dev_count = (int)out_count;
            }
            else
            {
                ui->dev_count = 0;
            }
        }

        // Devices list
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Devices (libusb):", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 120, 1);
        if (nk_group_begin(ctx, "devices", NK_WINDOW_BORDER))
        {
            for (int i = 0; i < ui->dev_count; ++i)
            {
                char line[256];
                snprintf(line, sizeof(line), "[%d] %04X:%04X %s %s %s",
                         i,
                         (unsigned)ui->devlist[i].vid,
                         (unsigned)ui->devlist[i].pid,
                         ui->devlist[i].manufacturer,
                         ui->devlist[i].product,
                         ui->devlist[i].serial);
                nk_layout_row_dynamic(ctx, 16, 1);
                nk_label(ctx, line, NK_TEXT_LEFT);
            }
            nk_group_end(ctx);
        }

        nk_layout_row_dynamic(ctx, 26, 1);
        nk_label(ctx, "Input (send as bytes / raw):", NK_TEXT_LEFT);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, ui->input_line, sizeof(ui->input_line), nk_filter_default);

        nk_layout_row_dynamic(ctx, 28, 2);
        if (nk_button_label(ctx, "Send"))
        {
            const char *s = ui->input_line;
            if (s[0])
            {
                worker_send(ui->worker, (const uint8_t *)s, strlen(s));
                ui->input_line[0] = 0;
            }
        }
        nk_checkbox_label(ctx, "Auto-scroll", &ui->auto_scroll);

        // Console
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Console (RX):", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 200, 1);
        if (nk_group_begin(ctx, "console", NK_WINDOW_BORDER))
        {
            uint8_t tmp[512];
            char line[2048];
            size_t got = 0;
            do
            {
                got = rb_pop(ui->worker->rxbuf, tmp, sizeof(tmp));
                if (got)
                {
                    to_hex_line(tmp, got, line, sizeof(line));
                    nk_layout_row_dynamic(ctx, 16, 1);
                    nk_label(ctx, line, NK_TEXT_LEFT);
                }
            } while (got);

            if (ui->auto_scroll)
            {
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

// USB Data Reader Page
static void ui_usb_reader_page(app_ui_t *ui, struct nk_context *ctx)
{
    if (nk_begin(ctx, "USB Data Reader", nk_rect(10, 70, 780, 500),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "Select USB Device for Reading:", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 150, 1);
        if (nk_group_begin(ctx, "reader_devices", NK_WINDOW_BORDER))
        {
            for (int i = 0; i < ui->dev_count; ++i)
            {
                char line[256];
                snprintf(line, sizeof(line), "[%d] %04X:%04X %s %s %s",
                         i,
                         (unsigned)ui->devlist[i].vid,
                         (unsigned)ui->devlist[i].pid,
                         ui->devlist[i].manufacturer,
                         ui->devlist[i].product,
                         ui->devlist[i].serial);
                nk_layout_row_dynamic(ctx, 20, 1);
                int is_selected = (ui->selected_reader_device == i);
                if (nk_selectable_label(ctx, line, NK_TEXT_LEFT, &is_selected))
                {
                    ui->selected_reader_device = i;
                }
            }
            nk_group_end(ctx);
        }
        
        nk_layout_row_dynamic(ctx, 30, 3);
        if (nk_button_label(ctx, "Connect"))
        {
            ui->reader_connected = 1;
            // TODO: Implement actual USB connection
        }
        if (nk_button_label(ctx, "Disconnect"))
        {
            ui->reader_connected = 0;
            // TODO: Implement actual USB disconnection
        }
        if (nk_button_label(ctx, "Refresh Devices"))
        {
            size_t out_count = 0;
            if (FTDI_OK == ftdi_enumerate_devices(ui->devlist, (size_t)32, &out_count))
            {
                ui->dev_count = (int)out_count;
            }
            else
            {
                ui->dev_count = 0;
            }
        }
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Connection Status:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 20, 1);
        if (ui->reader_connected)
        {
            nk_label_colored(ctx, "Connected", NK_TEXT_LEFT, nk_rgb(0, 255, 0));
        }
        else
        {
            nk_label_colored(ctx, "Disconnected", NK_TEXT_LEFT, nk_rgb(255, 0, 0));
        }
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Data Output:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 200, 1);
        if (nk_group_begin(ctx, "reader_output", NK_WINDOW_BORDER))
        {
            nk_layout_row_dynamic(ctx, 16, 1);
            nk_label(ctx, ui->reader_output, NK_TEXT_LEFT);
            nk_group_end(ctx);
        }
    }
    nk_end(ctx);
}

// USB Data Writer Page
static void ui_usb_writer_page(app_ui_t *ui, struct nk_context *ctx)
{
    if (nk_begin(ctx, "USB Data Writer", nk_rect(10, 70, 780, 500),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "Select USB Device for Writing:", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 150, 1);
        if (nk_group_begin(ctx, "writer_devices", NK_WINDOW_BORDER))
        {
            for (int i = 0; i < ui->dev_count; ++i)
            {
                char line[256];
                snprintf(line, sizeof(line), "[%d] %04X:%04X %s %s %s",
                         i,
                         (unsigned)ui->devlist[i].vid,
                         (unsigned)ui->devlist[i].pid,
                         ui->devlist[i].manufacturer,
                         ui->devlist[i].product,
                         ui->devlist[i].serial);
                nk_layout_row_dynamic(ctx, 20, 1);
                int is_selected = (ui->selected_writer_device == i);
                if (nk_selectable_label(ctx, line, NK_TEXT_LEFT, &is_selected))
                {
                    ui->selected_writer_device = i;
                }
            }
            nk_group_end(ctx);
        }
        
        nk_layout_row_dynamic(ctx, 30, 3);
        if (nk_button_label(ctx, "Connect"))
        {
            ui->writer_connected = 1;
            // TODO: Implement actual USB connection
        }
        if (nk_button_label(ctx, "Disconnect"))
        {
            ui->writer_connected = 0;
            // TODO: Implement actual USB disconnection
        }
        if (nk_button_label(ctx, "Refresh Devices"))
        {
            size_t out_count = 0;
            if (FTDI_OK == ftdi_enumerate_devices(ui->devlist, (size_t)32, &out_count))
            {
                ui->dev_count = (int)out_count;
            }
            else
            {
                ui->dev_count = 0;
            }
        }
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Connection Status:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 20, 1);
        if (ui->writer_connected)
        {
            nk_label_colored(ctx, "Connected", NK_TEXT_LEFT, nk_rgb(0, 255, 0));
        }
        else
        {
            nk_label_colored(ctx, "Disconnected", NK_TEXT_LEFT, nk_rgb(255, 0, 0));
        }
        
        // Checksum and Offset inputs
        nk_layout_row_dynamic(ctx, 30, 2);
        nk_label(ctx, "Checksum:", NK_TEXT_LEFT);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, ui->checksum_input, sizeof(ui->checksum_input), nk_filter_default);
        
        nk_layout_row_dynamic(ctx, 30, 2);
        nk_label(ctx, "Offset:", NK_TEXT_LEFT);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, ui->offset_input, sizeof(ui->offset_input), nk_filter_default);
        
        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_button_label(ctx, "Write Data"))
        {
            // TODO: Implement data writing with checksum and offset
            snprintf(ui->writer_output, sizeof(ui->writer_output), 
                    "Writing data with checksum: %s, offset: %s", 
                    ui->checksum_input, ui->offset_input);
        }
        if (nk_button_label(ctx, "Clear Output"))
        {
            ui->writer_output[0] = '\0';
        }
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Output:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 150, 1);
        if (nk_group_begin(ctx, "writer_output", NK_WINDOW_BORDER))
        {
            nk_layout_row_dynamic(ctx, 16, 1);
            nk_label(ctx, ui->writer_output, NK_TEXT_LEFT);
            nk_group_end(ctx);
        }
    }
    nk_end(ctx);
}

void ui_frame(app_ui_t *ui, void *nkctx)
{
    struct nk_context *ctx = (struct nk_context *)nkctx;

    // Draw navigation menu
    ui_navigation_menu(ui, ctx);
    
    // Draw current page based on selection
    switch (ui->current_page)
    {
        case UI_PAGE_OBD_VIEWER:
            ui_obd_viewer_page(ui, ctx);
            break;
        case UI_PAGE_USB_READER:
            ui_usb_reader_page(ui, ctx);
            break;
        case UI_PAGE_USB_WRITER:
            ui_usb_writer_page(ui, ctx);
            break;
    }
}
