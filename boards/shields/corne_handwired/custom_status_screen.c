#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

/* Step 2 debug build: same as step 1 but WITHOUT lv_disp_set_rotation().
 * Step 1 (rotation + label) hung the central, so the rotation call is the
 * suspect. If this build boots and shows "AS" horizontally, rotation is the
 * bug — we'll move rotation to the SSD1306 driver level instead. */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *logo = lv_label_create(screen);
    lv_label_set_text(logo, "AS");
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 4);

    return screen;
}
