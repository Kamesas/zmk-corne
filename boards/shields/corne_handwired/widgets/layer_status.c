#include <zephyr/kernel.h>
#include <stdio.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#include "layer_status.h"

struct layer_status_state {
    uint8_t index;
};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void set_layer_text(struct zmk_widget_layer_status *widget,
                           struct layer_status_state state) {
    char text[4];
    snprintf(text, sizeof(text), "%d", state.index);
    lv_label_set_text(widget->label, text);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_layer_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_layer_text(widget, state);
    }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    return (struct layer_status_state){
        .index = zmk_keymap_highest_layer_active(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state,
                            layer_status_update_cb, layer_status_get_state)
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

int zmk_widget_layer_status_init(struct zmk_widget_layer_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 32, 40);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, LV_PART_MAIN);

    widget->label = lv_label_create(widget->obj);
    lv_label_set_text(widget->label, "0");
    lv_obj_align(widget->label, LV_ALIGN_CENTER, 0, 0);

    sys_slist_append(&widgets, &widget->node);
    widget_layer_status_init();
    return 0;
}

lv_obj_t *zmk_widget_layer_status_obj(struct zmk_widget_layer_status *widget) {
    return widget->obj;
}
