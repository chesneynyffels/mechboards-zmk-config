/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <zmk/event_manager.h>

struct nyffels_display_art_changed {
    bool gaming;
};

ZMK_EVENT_DECLARE(nyffels_display_art_changed);

bool nyffels_display_art_is_game(void);
void nyffels_display_art_set_game(bool gaming);
