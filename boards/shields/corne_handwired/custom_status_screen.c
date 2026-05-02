#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

/* Step 5 debug: label + font-default fix, NO rotation.
 * Step 4 (rotation + label + font default) still hung. If this boots and
 * shows "AS" horizontally, rotation is also a hang trigger. If this hangs,
 * the font-default Kconfig override didn't take effect. */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *logo = lv_label_create(screen);
    lv_label_set_text(logo, "AS");
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 4);

    return screen;
}
