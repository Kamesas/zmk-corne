#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

/* Step 7: explicit black bg + white text so the label is visible on a
 * monochrome panel. Step 6 booted but rendered only the panel's background
 * — default LVGL theme uses white-on-white under LV_COLOR_DEPTH_1. */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *logo = lv_label_create(screen);
    lv_label_set_text(logo, "AS");
    lv_obj_set_style_text_color(logo, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 4);

    return screen;
}
