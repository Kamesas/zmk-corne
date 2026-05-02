#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

/* Step 6: label only, no rotation, with LV_Z_MEM_POOL_SIZE bumped to 8192.
 * Step 5 (label only, default 4096-byte pool) hung. Web research showed
 * upstream nice_view sets the pool to 8192 for custom screens. If this
 * boots and shows "AS" horizontally, memory pool was the bug. Rotation
 * stays off — rotation hazard is independent and we'll address it after
 * confirming the basic label path. */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *logo = lv_label_create(screen);
    lv_label_set_text(logo, "AS");
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 4);

    return screen;
}
