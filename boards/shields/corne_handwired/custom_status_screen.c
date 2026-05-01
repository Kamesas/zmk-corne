#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

#include "widgets/peripheral_status.h"

static struct zmk_widget_peripheral_status peripheral_status_widget;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    /* Rotate framebuffer 90° → logical canvas becomes 32 wide × 128 tall.
     * If display reads upside down once flashed, change LV_DISP_ROTATION_90
     * to LV_DISP_ROTATION_270 here. */
    lv_disp_set_rotation(NULL, LV_DISP_ROTATION_90);

    lv_obj_t *logo = lv_label_create(screen);
    lv_label_set_text(logo, "AS");
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 4);

    zmk_widget_peripheral_status_init(&peripheral_status_widget, screen);
    lv_obj_align(zmk_widget_peripheral_status_obj(&peripheral_status_widget),
                 LV_ALIGN_BOTTOM_MID, 0, -4);

    return screen;
}
