/*
 * SPDX-License-Identifier: MIT
 *
 * Shared canvas helpers for the corne_handwired status widget.
 * Pattern lifted from zmk/app/boards/shields/nice_view/widgets/util.h,
 * trimmed to what the 128x32 SSD1306 build needs.
 */

#pragma once

#include <lvgl.h>
#include <zmk/endpoints.h>

#define CANVAS_SIZE 32
#define CANVAS_COLOR_FORMAT LV_COLOR_FORMAT_L8
#define CANVAS_BUF_SIZE                                                                            \
    LV_CANVAS_BUF_SIZE(CANVAS_SIZE, CANVAS_SIZE, LV_COLOR_FORMAT_GET_BPP(CANVAS_COLOR_FORMAT),     \
                       LV_DRAW_BUF_STRIDE_ALIGN)

// lv_draw_sw_rotate with LV_DISPLAY_ROTATION_90 inverts L8 pixel values (0↔255).
// Swapping FG/BG compensates: black (L8=0) inverts to 255=bright; white (L8=255)
// inverts to 0=dark. Do NOT use for widget container styling — that path bypasses
// the canvas pipeline and renders 1bpp directly without inversion.
#define LVGL_FOREGROUND lv_color_black()
#define LVGL_BACKGROUND lv_color_white()

struct status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
    uint8_t layer_index;
    const char *layer_label;
    bool caps_lock;
};

void rotate_canvas(lv_obj_t *canvas);

void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align);
void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color);

void canvas_draw_rect(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h,
                      lv_draw_rect_dsc_t *draw_dsc);
void canvas_draw_text(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t max_w,
                      lv_draw_label_dsc_t *draw_dsc, const char *txt);
