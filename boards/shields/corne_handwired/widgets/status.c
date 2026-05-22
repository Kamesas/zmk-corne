/*
 * SPDX-License-Identifier: MIT
 *
 * Vertical status widget for the left-half OLED.
 * Three 32x32 canvases drawn upright, then rotated 270° onto the
 * 128x32 framebuffer so the screen reads vertically.
 *
 *   Top canvas:    endpoint icon + caps lock icon, both Montserrat 14
 *                  (caps only shown when active).
 *   Middle canvas: large active-layer digit (Montserrat 28).
 *   Bottom canvas: L XX / R XX battery percentages (Montserrat 14).
 *                  "XX" replaced with "--" when no cell is present.
 *
 * Central-only: peripheral build cannot link layer events
 * (zmk/app/CMakeLists.txt gates keymap.c + layer_state_changed.c on
 *  CONFIG_ZMK_SPLIT_ROLE_CENTRAL), so this file is excluded on the
 * peripheral. The right half has no display anyway.
 */

#include <zephyr/kernel.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/hid_indicators.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/battery.h>

#include "status.h"

// Standard HID Keyboard LED report bit for Caps Lock.
#define HID_LED_CAPS_LOCK_BIT 0x02

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};

struct layer_status_state {
    zmk_keymap_layer_index_t index;
    const char *label;
};

struct hid_indicators_state {
    bool caps_lock;
};

struct battery_status_state {
    uint8_t central;
    uint8_t peripheral;
};

// Peripheral battery has no global query API — cache the last reported value
// so the widget can re-render correctly when other listeners fire.
static uint8_t last_peripheral_battery;

static const char *endpoint_icon(const struct status_state *state) {
    switch (state->selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        return LV_SYMBOL_USB;
    case ZMK_TRANSPORT_BLE:
        if (state->active_profile_bonded) {
            return state->active_profile_connected ? LV_SYMBOL_BLUETOOTH : LV_SYMBOL_CLOSE;
        }
        return LV_SYMBOL_SETTINGS;
    default:
        return LV_SYMBOL_CLOSE;
    }
}

static void draw_top(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    // Endpoint icon, top half
    canvas_draw_text(canvas, 0, 1, CANVAS_SIZE, &label_dsc, endpoint_icon(state));

    // Caps lock, bottom half (only when active)
    if (state->caps_lock) {
        canvas_draw_text(canvas, 0, 17, CANVAS_SIZE, &label_dsc, LV_SYMBOL_UP);
    }

    rotate_canvas(canvas);
}

static void draw_middle(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_28, LV_TEXT_ALIGN_CENTER);

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    char text[3] = {};
    snprintf(text, sizeof(text), "%d", state->layer_index);
    canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc, text);

    rotate_canvas(canvas);
}

static void format_battery(char *out, size_t len, char prefix, uint8_t pct) {
    if (pct == 0) {
        snprintf(out, len, "%c--", prefix);
    } else {
        snprintf(out, len, "%c%u", prefix, pct);
    }
}

static void draw_bottom(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);

    char left[5];
    char right[5];
    format_battery(left, sizeof(left), 'L', state->battery_central);
    format_battery(right, sizeof(right), 'R', state->battery_peripheral);
    canvas_draw_text(canvas, 0, 1, CANVAS_SIZE, &label_dsc, left);
    canvas_draw_text(canvas, 0, 17, CANVAS_SIZE, &label_dsc, right);

    rotate_canvas(canvas);
}

static void set_output_status(struct zmk_widget_status *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;

    draw_top(widget->obj, &widget->state);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoint_get_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;

    draw_middle(widget->obj, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = index, .label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index))};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)
ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

static void set_hid_indicators(struct zmk_widget_status *widget,
                               struct hid_indicators_state state) {
    widget->state.caps_lock = state.caps_lock;

    draw_top(widget->obj, &widget->state);
}

static void hid_indicators_update_cb(struct hid_indicators_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_hid_indicators(widget, state); }
}

static struct hid_indicators_state hid_indicators_get_state(const zmk_event_t *_eh) {
    return (struct hid_indicators_state){
        .caps_lock = (zmk_hid_indicators_get_current_profile() & HID_LED_CAPS_LOCK_BIT) != 0,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_hid_indicators, struct hid_indicators_state,
                            hid_indicators_update_cb, hid_indicators_get_state)
ZMK_SUBSCRIPTION(widget_hid_indicators, zmk_hid_indicators_changed);

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)

static void set_battery_status(struct zmk_widget_status *widget, struct battery_status_state state) {
    widget->state.battery_central = state.central;
    widget->state.battery_peripheral = state.peripheral;

    draw_bottom(widget->obj, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    // Peripheral side: prefer the event payload if this fired from a peripheral
    // battery update; otherwise fall through to the cached value. Central side
    // always queries the live SOC.
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING) ||                             \
    IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY)
    const struct zmk_peripheral_battery_state_changed *pev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (pev != NULL) {
        last_peripheral_battery = pev->state_of_charge;
    }
#else
    ARG_UNUSED(eh);
#endif
    return (struct battery_status_state){
        .central = zmk_battery_state_of_charge(),
        .peripheral = last_peripheral_battery,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING) ||                             \
    IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_peripheral_battery_state_changed);
#endif

#endif // CONFIG_ZMK_BATTERY_REPORTING

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 32);
    lv_obj_set_style_bg_color(widget->obj, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);

    lv_obj_t *middle = lv_canvas_create(widget->obj);
    lv_obj_align(middle, LV_ALIGN_TOP_LEFT, 48, 0);
    lv_canvas_set_buffer(middle, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);

    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_LEFT, 96, 0);
    lv_canvas_set_buffer(bottom, widget->cbuf3, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);

    sys_slist_append(&widgets, &widget->node);
    widget_output_status_init();
    widget_layer_status_init();
    widget_hid_indicators_init();
#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    widget_battery_status_init();
#endif

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
