/* SPDX-License-Identifier: MIT */
#include <lvgl.h>
#include <zephyr/kernel.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <nyffels/display_art.h>

LV_IMG_DECLARE(nyffels_logo);
LV_IMG_DECLARE(nyffels_controller);

static lv_obj_t *art_image;

static void art_update_cb(bool gaming) {
    const lv_img_dsc_t *image = gaming ? &nyffels_controller : &nyffels_logo;
    if (art_image && lv_img_get_src(art_image) != image) {
        lv_img_set_src(art_image, image);
    }
}

static bool art_get_state(const zmk_event_t *eh) {
    return nyffels_display_art_is_game();
}

static int art_connection_listener(const zmk_event_t *eh) {
    const struct zmk_split_peripheral_status_changed *ev =
        as_zmk_split_peripheral_status_changed(eh);
    if (ev && !ev->connected) {
        /* Disconnected: show the brand image until the left half resyncs. */
        nyffels_display_art_set_game(false);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(nyffels_art_connection, art_connection_listener);
ZMK_SUBSCRIPTION(nyffels_art_connection, zmk_split_peripheral_status_changed);

ZMK_DISPLAY_WIDGET_LISTENER(nyffels_art_display, bool, art_update_cb, art_get_state);
ZMK_SUBSCRIPTION(nyffels_art_display, nyffels_display_art_changed);

/* Only this peripheral decoration entry point is wrapped. GEM still creates
 * its normal battery and connection widgets, and owns the screen lifetime. */
void __wrap_draw_animation(lv_obj_t *parent) {
    art_image = lv_img_create(parent);
    /* Native display: 160 x 68, physically mounted in portrait. The lower
     * 124 portrait rows map to native x=0..123. Status keeps its top 36 rows. */
    lv_obj_align(art_image, LV_ALIGN_TOP_LEFT, 0, 0);
    nyffels_art_display_init();
}
