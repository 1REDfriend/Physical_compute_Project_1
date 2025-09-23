#ifndef UI_H
#define UI_H

#include <stddef.h>
#include <stdint.h>
#include "worker.h"

#ifdef __cplusplus
extern "C" {
#endif

// UI Page enumeration
typedef enum {
    UI_PAGE_OBD_VIEWER = 0,    // หน้า OBD-II Data Viewer
    UI_PAGE_USB_READER = 1,    // หน้า USB Data Reader  
    UI_PAGE_USB_WRITER = 2     // หน้า USB Data Writer
} ui_page_t;

typedef struct app_ui {
    worker_t* worker;
    int       want_quit;
    
    // Current page
    ui_page_t current_page;

    // UI state
    int driver_select;   // 0=stub, 1=d2xx
    char input_line[256];
    int  auto_scroll;

    // Enumerated devices (read-only list for display)
    ftdi_device_info_t devlist[32];
    int                dev_count;
    
    // USB Reader page state
    int selected_reader_device;
    int reader_connected;
    char reader_output[4096];
    
    // USB Writer page state  
    int selected_writer_device;
    int writer_connected;
    char checksum_input[32];
    char offset_input[32];
    char writer_output[4096];
} app_ui_t;

void ui_init(app_ui_t* ui, worker_t* w);
void ui_frame(app_ui_t* ui, void* nk_ctx); // nk_ctx = struct nk_context*

#ifdef __cplusplus
}
#endif

#endif // UI_H
