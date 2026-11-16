/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT nyffels_display_art

#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <nyffels/display_art.h>

static atomic_t game_mode;

ZMK_EVENT_IMPL(nyffels_display_art_changed);

bool nyffels_display_art_is_game(void) { return atomic_get(&game_mode) != 0; }

void nyffels_display_art_set_game(bool gaming) {
    atomic_set(&game_mode, gaming);
    /* Repeated reconnect messages must also notify the UI: the first message
     * could arrive before display initialization finished. The widget skips
     * drawing when the requested image is already selected. */
    raise_nyffels_display_art_changed((struct nyffels_display_art_changed){.gaming = gaming});
}

static int art_pressed(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    nyffels_display_art_set_game(binding->param1 != 0);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int art_released(struct zmk_behavior_binding *binding,
                        struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api art_api = {
    .binding_pressed = art_pressed,
    .binding_released = art_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &art_api);
