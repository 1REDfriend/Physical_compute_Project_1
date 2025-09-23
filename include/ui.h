#ifndef UI_H
#define UI_H

#include <stddef.h>
#include <stdint.h>
#include "worker.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct app_ui {
    worker_t* worker;
    int       want_quit;

    // UI state
    int driver_select;   // 0=stub, 1=d2xx
    char input_line[256];
    int  auto_scroll;

    // Enumerated devices (read-only list for display)
    ftdi_device_info_t devlist[32];
    int                dev_count;
} app_ui_t;

void ui_init(app_ui_t* ui, worker_t* w);
void ui_frame(app_ui_t* ui, void* nk_ctx); // nk_ctx = struct nk_context*

#ifdef __cplusplus
}
#endif

#endif // UI_H
