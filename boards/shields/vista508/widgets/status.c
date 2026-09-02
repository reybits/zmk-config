/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include "status.h"
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

#if defined(CONFIG_VISTA508_WIDGET_TOP_WPM)
#include <zmk/wpm.h>
#include <zmk/events/wpm_state_changed.h>
#endif

#if defined(CONFIG_VISTA508_WIDGET_TOP_MODS_ART)
#include <zmk/hid.h>
#include <zmk/events/keycode_state_changed.h>
#endif

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};

struct layer_status_state {
    uint8_t index;
    const char *label;
};

#if defined(CONFIG_VISTA508_WIDGET_TOP_WPM)
struct wpm_status_state {
    uint8_t wpm;
};
#endif

#if defined(CONFIG_VISTA508_WIDGET_TOP_MODS_ART)
struct mods_status_state {
    uint8_t mods;
};
#endif

// Header strip shared by both variants: battery + connection icon in the
// top ~20 px of the canvas. Kept as its own helper so the MODS_ART variant
// can redraw it without touching the art region below.
static void draw_header(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_bg_dsc;
    init_rect_dsc(&rect_bg_dsc, LVGL_BACKGROUND);

    canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, 20, &rect_bg_dsc);

    draw_battery(canvas, state);

    char output_text[10] = {};
    switch (state->selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        strcat(output_text, LV_SYMBOL_USB);
        break;
    case ZMK_TRANSPORT_BLE:
        if (state->active_profile_bonded) {
            if (state->active_profile_connected) {
                strcat(output_text, LV_SYMBOL_OK);
            } else {
                strcat(output_text, LV_SYMBOL_CLOSE);
            }
        } else {
            strcat(output_text, LV_SYMBOL_SETTINGS);
        }
        break;
    default:
        // transport only holds USB or BLE when healthy; anything else
        // is a corrupted snapshot. Make it visible instead of blank.
        strcat(output_text, "?");
        break;
    }
    canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc, output_text);
}

#if defined(CONFIG_VISTA508_WIDGET_TOP_WPM)

// Original top-canvas layout: header + WPM graph box occupying most of the
// visible strip. Full-canvas clear on every update.
static void draw_top(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    lv_draw_label_dsc_t label_dsc_wpm;
    init_label_dsc(&label_dsc_wpm, LVGL_FOREGROUND, &lv_font_montserrat_12, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_bg_dsc;
    init_rect_dsc(&rect_bg_dsc, LVGL_BACKGROUND);
    lv_draw_line_dsc_t line_thick_dsc;
    init_line_dsc(&line_thick_dsc, LVGL_FOREGROUND, 2);
    lv_draw_arc_dsc_t arc_dsc;
    init_arc_dsc(&arc_dsc, LVGL_FOREGROUND, 2);
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);

    canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_bg_dsc);

    draw_header(canvas, state);

    const int WPM_HEIGHT = 82;
    const uint8_t cornerRadius = 4;
    const uint8_t yOffset = 21;
    lv_point_t boxPoints[2];
    boxPoints[0].x = 1 + cornerRadius;
    boxPoints[0].y = yOffset;
    boxPoints[1].x = CANVAS_SIZE - cornerRadius - 1;
    boxPoints[1].y = yOffset;
    canvas_draw_line(canvas, boxPoints, 2, &line_thick_dsc);
    canvas_draw_arc(canvas, boxPoints[0].x, boxPoints[0].y + cornerRadius, cornerRadius, 180, 270, &arc_dsc);
    canvas_draw_arc(canvas, boxPoints[1].x, boxPoints[1].y + cornerRadius, cornerRadius, 270, 0, &arc_dsc);
    boxPoints[0].y = yOffset + WPM_HEIGHT;
    boxPoints[1].y = yOffset + WPM_HEIGHT;
    canvas_draw_line(canvas, boxPoints, 2, &line_thick_dsc);
    canvas_draw_arc(canvas, boxPoints[0].x, boxPoints[0].y - cornerRadius, cornerRadius, 90, 180, &arc_dsc);
    canvas_draw_arc(canvas, boxPoints[1].x, boxPoints[1].y - cornerRadius, cornerRadius, 0, 90, &arc_dsc);
    boxPoints[0].x = 1;
    boxPoints[0].y = cornerRadius + yOffset;
    boxPoints[1].x = 1;
    boxPoints[1].y = yOffset + WPM_HEIGHT - cornerRadius;
    canvas_draw_line(canvas, boxPoints, 2, &line_thick_dsc);
    boxPoints[0].x = CANVAS_SIZE - 1;
    boxPoints[1].x = CANVAS_SIZE - 1;
    canvas_draw_line(canvas, boxPoints, 2, &line_thick_dsc);

    char wpm_text[6] = {};
    snprintf(wpm_text, sizeof(wpm_text), "%d", state->wpm[WPM_SAMPLES - 1]);
    canvas_draw_text(canvas, CANVAS_SIZE - 29, 3 + WPM_HEIGHT, 24, &label_dsc_wpm, wpm_text);

    int max = 0;
    int min = 256;
    for (int i = 0; i < WPM_SAMPLES; i++) {
        if (state->wpm[i] > max) {
            max = state->wpm[i];
        }
        if (state->wpm[i] < min) {
            min = state->wpm[i];
        }
    }
    int range = max - min;
    if (range == 0) {
        range = 1;
    }

    const int WPM_COLUMN_WIDTH = (CANVAS_SIZE - 4) / WPM_SAMPLES;
    lv_point_t points[WPM_SAMPLES];
    for (int i = 0; i < WPM_SAMPLES; i++) {
        points[i].x = 3 + i * WPM_COLUMN_WIDTH;
        points[i].y = 20 + WPM_HEIGHT - (state->wpm[i] - min) * (WPM_HEIGHT - 7) / range;
    }
    canvas_draw_line(canvas, points, WPM_SAMPLES, &line_dsc);
}

#elif defined(CONFIG_VISTA508_WIDGET_TOP_MODS_ART)

// Split top canvas (visible rows 0..111):
//   0..19    header  - battery + connection icon
//   20..87   art     - static procedural landscape, drawn once at init
//   88..111  mods    - four boxes tracking held-modifier state
#define ART_Y_TOP  20
#define ART_Y_BOT  87
#define MODS_Y_TOP 88
#define MODS_Y_BOT 111

// Simple hand-picked landscape: crescent-ish moon, a handful of stars, a
// mountain silhouette and a horizon. Everything is drawn from primitives -
// no image asset, no flash cost beyond a couple of coordinate arrays.
static void draw_art(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t rect_bg_dsc;
    init_rect_dsc(&rect_bg_dsc, LVGL_BACKGROUND);
    lv_draw_rect_dsc_t rect_fg_dsc;
    init_rect_dsc(&rect_fg_dsc, LVGL_FOREGROUND);
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);
    lv_draw_line_dsc_t line_thick_dsc;
    init_line_dsc(&line_thick_dsc, LVGL_FOREGROUND, 2);
    lv_draw_arc_dsc_t arc_moon_full;
    init_arc_dsc(&arc_moon_full, LVGL_FOREGROUND, 10);
    lv_draw_arc_dsc_t arc_moon_cut;
    init_arc_dsc(&arc_moon_cut, LVGL_BACKGROUND, 10);

    canvas_draw_rect(canvas, 0, ART_Y_TOP, CANVAS_SIZE, ART_Y_BOT - ART_Y_TOP + 1, &rect_bg_dsc);

    // Crescent: solid disc at (108, 38) minus another disc shifted right.
    canvas_draw_arc(canvas, 108, 38, 5, 0, 359, &arc_moon_full);
    canvas_draw_arc(canvas, 113, 37, 5, 0, 359, &arc_moon_cut);

    // Stars above the mountains.
    static const struct { uint8_t x, y; } stars[] = {
        {18, 30}, {40, 26}, {55, 33}, {72, 28}, {88, 34}, {130, 55},
    };
    for (unsigned i = 0; i < ARRAY_SIZE(stars); i++) {
        canvas_draw_rect(canvas, stars[i].x, stars[i].y, 2, 2, &rect_fg_dsc);
    }

    // Mountain range polyline.
    static const lv_point_t peaks[] = {
        {0,   82},
        {15,  70},
        {28,  76},
        {46,  57},
        {58,  70},
        {72,  62},
        {88,  73},
        {104, 55},
        {120, 68},
        {132, 62},
        {144, 78},
    };
    canvas_draw_line(canvas, peaks, ARRAY_SIZE(peaks), &line_thick_dsc);

    // Horizon.
    static const lv_point_t horizon[] = {{2, 85}, {142, 85}};
    canvas_draw_line(canvas, horizon, 2, &line_dsc);
}

// Four boxes ~34x20 with a 2 px gap, centered horizontally. Active
// modifiers get a filled rectangle with a background-colour label; the
// rest get a 1 px outline drawn as four lines (LVGL's rect descriptor
// has no border-only shortcut, so this is the cleanest way).
static void draw_mods(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_rect_dsc_t rect_bg_dsc;
    init_rect_dsc(&rect_bg_dsc, LVGL_BACKGROUND);
    lv_draw_rect_dsc_t rect_fg_dsc;
    init_rect_dsc(&rect_fg_dsc, LVGL_FOREGROUND);
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);
    lv_draw_label_dsc_t label_fg_dsc;
    init_label_dsc(&label_fg_dsc, LVGL_FOREGROUND, &lv_font_montserrat_12, LV_TEXT_ALIGN_CENTER);
    lv_draw_label_dsc_t label_bg_dsc;
    init_label_dsc(&label_bg_dsc, LVGL_BACKGROUND, &lv_font_montserrat_12, LV_TEXT_ALIGN_CENTER);

    static const char *labels[4] = {"SFT", "CTL", "ALT", "GUI"};
    // Combined L | R bit masks from the HID modifier byte.
    static const uint8_t masks[4] = {
        0x02 | 0x20,  // Shift
        0x01 | 0x10,  // Ctrl
        0x04 | 0x40,  // Alt / Option
        0x08 | 0x80,  // GUI / Cmd
    };

    const int box_h = 20;
    const int box_w = 34;
    const int gap = 2;
    const int total_w = 4 * box_w + 3 * gap;
    const int start_x = (CANVAS_SIZE - total_w) / 2;

    canvas_draw_rect(canvas, 0, MODS_Y_TOP, CANVAS_SIZE,
                     MODS_Y_BOT - MODS_Y_TOP + 1, &rect_bg_dsc);

    for (int i = 0; i < 4; i++) {
        int x = start_x + i * (box_w + gap);
        int y = MODS_Y_TOP + 1;
        bool active = (state->mods & masks[i]) != 0;

        if (active) {
            canvas_draw_rect(canvas, x, y, box_w, box_h, &rect_fg_dsc);
            canvas_draw_text(canvas, x, y + 2, box_w, &label_bg_dsc, labels[i]);
        } else {
            lv_point_t top_line[]   = {{x, y}, {x + box_w - 1, y}};
            lv_point_t bot_line[]   = {{x, y + box_h - 1}, {x + box_w - 1, y + box_h - 1}};
            lv_point_t left_line[]  = {{x, y}, {x, y + box_h - 1}};
            lv_point_t right_line[] = {{x + box_w - 1, y}, {x + box_w - 1, y + box_h - 1}};
            canvas_draw_line(canvas, top_line, 2, &line_dsc);
            canvas_draw_line(canvas, bot_line, 2, &line_dsc);
            canvas_draw_line(canvas, left_line, 2, &line_dsc);
            canvas_draw_line(canvas, right_line, 2, &line_dsc);
            canvas_draw_text(canvas, x, y + 2, box_w, &label_fg_dsc, labels[i]);
        }
    }
}

// draw_top for the MODS_ART variant only redraws the header strip.
// The art zone lives untouched below it, drawn once by draw_art() at
// widget init; the mods zone is refreshed by its own listener.
static void draw_top(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    draw_header(canvas, state);
}

#endif

static void draw_middle(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);
    lv_draw_arc_dsc_t arc_dsc;
    init_arc_dsc(&arc_dsc, LVGL_FOREGROUND, 2);
    lv_draw_arc_dsc_t arc_dsc_thin;
    init_arc_dsc(&arc_dsc_thin, LVGL_FOREGROUND, 1);
    lv_draw_arc_dsc_t arc_dsc_filled;
    init_arc_dsc(&arc_dsc_filled, LVGL_FOREGROUND, 9);
    lv_draw_arc_dsc_t arc_dsc_filled_background;
    init_arc_dsc(&arc_dsc_filled_background, LVGL_BACKGROUND, 9);
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 2);
    lv_draw_line_dsc_t line_thick_dsc;
    init_line_dsc(&line_thick_dsc, LVGL_FOREGROUND, 5);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);
    lv_draw_label_dsc_t status_dsc;
    init_label_dsc(&status_dsc, LVGL_FOREGROUND, &lv_font_montserrat_28, LV_TEXT_ALIGN_CENTER);
    lv_draw_label_dsc_t label_dsc_black;
    init_label_dsc(&label_dsc_black, LVGL_BACKGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);

    // Fill background
    canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    bool usingUsb = false;
    bool profileConnected = false;
    bool profileAdvertising = false;
    if (state->selected_endpoint.transport == ZMK_TRANSPORT_USB) {
        usingUsb = true;
    }
    if (state->active_profile_bonded) {
        if (state->active_profile_connected) {
            profileConnected = true;
        }
    } else {
        profileAdvertising = true;
    }

    if (usingUsb) {
        // Draw the pill outline
        canvas_draw_arc(canvas, 15, 14, 13, 90, 270, &arc_dsc);
        canvas_draw_arc(canvas, 95, 14, 13, 270, 90, &arc_dsc);
        lv_point_t points[2];
        points[0].x = 15;
        points[0].y = 2;
        points[1].x = 95;
        points[1].y = 2;
        canvas_draw_line(canvas, points, 2, &line_dsc);
        points[0].y = 26;
        points[1].y = 26;
        canvas_draw_line(canvas, points, 2, &line_dsc);
        // Draw the pill fill
        canvas_draw_arc(canvas, 15, 14, 9, 0, 359, &arc_dsc_filled);
        canvas_draw_arc(canvas, 95, 14, 9, 0, 359, &arc_dsc_filled);
        canvas_draw_rect(canvas, 15, 5, 80, 18, &rect_white_dsc);
        // Draw the label
        canvas_draw_text(canvas, 5, 4, 100, &label_dsc_black, "USB Out");
        // Draw the current profile indicator
        uint8_t xOffset = 129;
        uint8_t yOffset = 14;
        char profileNumber[4] = {};
        snprintf(profileNumber, sizeof(profileNumber), "%d", (uint8_t)state->active_profile_index + 1);
        if (profileConnected) {
            canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc);
            canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &arc_dsc_filled);
            canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc_black, profileNumber);
        }
        else if (profileAdvertising) {
            canvas_draw_text(canvas, xOffset - 8, yOffset - 15, 16, &status_dsc, LV_SYMBOL_SETTINGS);
            canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &arc_dsc_filled);
            canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc_black, profileNumber);
        }
        else {
            canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc_thin);
            canvas_draw_arc(canvas, xOffset, yOffset, 10, 0, 359, &arc_dsc_thin);
            canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc, profileNumber);
        }
        return;
    }

    // Draw circles
    uint8_t xOffset = 15;
    uint8_t yOffset = 15;
    for (int i = 0; i < 5; i++) {
        bool selected = i == state->active_profile_index;

        char label[2];
        snprintf(label, sizeof(label), "%d", i + 1);

        if (selected) {
            if (profileAdvertising) {
                canvas_draw_text(canvas, xOffset - 8, yOffset - 15, 16, &status_dsc, LV_SYMBOL_SETTINGS);
                canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &arc_dsc_filled);
                canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc_black, label);
            }
            else if (profileConnected) {
                canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc);
                canvas_draw_arc(canvas, xOffset, yOffset, 9, 0, 359, &arc_dsc_filled);
                canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc_black, label);
            }
            else {
                canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc_thin);
                canvas_draw_arc(canvas, xOffset, yOffset, 10, 0, 359, &arc_dsc_thin);
                canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc, label);
            }
        }
        else {
            canvas_draw_arc(canvas, xOffset, yOffset, 13, 0, 359, &arc_dsc);
            canvas_draw_text(canvas, xOffset - 8, yOffset - 11, 16, &label_dsc, label);
        }
        xOffset +=29;
    }
}

static void draw_bottom(lv_obj_t *widget, const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_18, LV_TEXT_ALIGN_CENTER);

    // Fill background
    canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Draw layer
    if (state->layer_label == NULL) {
        char text[10] = {};

        snprintf(text, sizeof(text), "LAYER %i", state->layer_index);

        canvas_draw_text(canvas, 0, 5, CANVAS_SIZE, &label_dsc, text);
    } else {
        canvas_draw_text(canvas, 0, 5, CANVAS_SIZE, &label_dsc, state->layer_label);
    }
}

static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;

    draw_top(widget->obj, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    return (struct battery_status_state){
        .level = zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

static void set_output_status(struct zmk_widget_status *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;

    draw_top(widget->obj, &widget->state);
    draw_middle(widget->obj, &widget->state);
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
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;

    draw_bottom(widget->obj, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    uint8_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){.index = index, .label = zmk_keymap_layer_name(index)};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

#if defined(CONFIG_VISTA508_WIDGET_TOP_WPM)

static void set_wpm_status(struct zmk_widget_status *widget, struct wpm_status_state state) {
    for (int i = 0; i < WPM_SAMPLES - 1; i++) {
        widget->state.wpm[i] = widget->state.wpm[i + 1];
    }
    widget->state.wpm[WPM_SAMPLES - 1] = state.wpm;

    draw_top(widget->obj, &widget->state);
}

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_status(widget, state); }
}

struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    return (struct wpm_status_state){.wpm = zmk_wpm_get_state()};
};

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state, wpm_status_update_cb,
                            wpm_status_get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

#endif  /* CONFIG_VISTA508_WIDGET_TOP_WPM */

#if defined(CONFIG_VISTA508_WIDGET_TOP_MODS_ART)

// Only redraws the modifier zone when the combined L|R modifier bitmap
// actually changes, so idle-time redraws are zero even though the
// listener subscribes to every keycode-state event.
static void set_mods_status(struct zmk_widget_status *widget,
                            struct mods_status_state state) {
    if (widget->state.mods == state.mods) {
        return;
    }
    widget->state.mods = state.mods;
    lv_obj_t *canvas = lv_obj_get_child(widget->obj, 0);
    draw_mods(canvas, &widget->state);
}

static void mods_status_update_cb(struct mods_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_mods_status(widget, state); }
}

static struct mods_status_state mods_status_get_state(const zmk_event_t *eh) {
    return (struct mods_status_state){.mods = zmk_hid_get_explicit_mods()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_mods_status, struct mods_status_state,
                            mods_status_update_cb, mods_status_get_state)
ZMK_SUBSCRIPTION(widget_mods_status, zmk_keycode_state_changed);

#endif  /* CONFIG_VISTA508_WIDGET_TOP_MODS_ART */

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 144, 168);
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);
    lv_obj_t *middle = lv_canvas_create(widget->obj);
    lv_obj_align(middle, LV_ALIGN_TOP_LEFT, 0, 112);
    lv_canvas_set_buffer(middle, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);
    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_LEFT, 0, 142);
    lv_canvas_set_buffer(bottom, widget->cbuf3, CANVAS_SIZE, CANVAS_SIZE, CANVAS_COLOR_FORMAT);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_output_status_init();
    widget_layer_status_init();
#if defined(CONFIG_VISTA508_WIDGET_TOP_WPM)
    widget_wpm_status_init();
#elif defined(CONFIG_VISTA508_WIDGET_TOP_MODS_ART)
    widget_mods_status_init();
    // The art zone is never touched by any listener; draw it once here
    // and it stays put for the lifetime of the widget.
    draw_art(top);
    // The mods listener's initial-state callback early-returns because
    // widget->state.mods and the freshly-fetched value are both zero on
    // boot, so nothing has painted the mods zone yet. Draw the empty
    // outlines explicitly so the strip is not left in canvas-default
    // (blank background) state until the first modifier press.
    draw_mods(top, &widget->state);
#endif

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
