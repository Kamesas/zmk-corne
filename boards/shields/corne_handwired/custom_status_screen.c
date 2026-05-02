#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

/* Step 3 debug build: bare screen, no label at all.
 * Step 2 (label only, no rotation) still hung. If this boots and shows a
 * blank display, the label/font path is the bug. If this still hangs, the
 * problem is in our status-screen integration itself (Kconfig, linkage). */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    return screen;
}
