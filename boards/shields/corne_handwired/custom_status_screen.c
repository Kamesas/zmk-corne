#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

/* Step 1 debug build: minimal vertical screen with just the "AS" label,
 * no layer widget. If this boots, the hang was in the widget. */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_disp_set_rotation(NULL, LV_DISP_ROTATION_90);

    lv_obj_t *logo = lv_label_create(screen);
    lv_label_set_text(logo, "AS");
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 4);

    return screen;
}
