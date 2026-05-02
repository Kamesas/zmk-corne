#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>

/* Step 8: explicit font on the label. Step 7 rendered missing-glyph
 * boxes — LVGL drew the label but the active font has no 'A'/'S' glyphs,
 * meaning the LV_FONT_DEFAULT Kconfig choice didn't actually take effect.
 * Bypass it by setting the font directly on the label. */

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *logo = lv_label_create(screen);
    lv_label_set_text(logo, "AS");
    lv_obj_set_style_text_color(logo, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(logo, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 4);

    return screen;
}
