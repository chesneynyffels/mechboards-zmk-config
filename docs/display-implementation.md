# Nyffels right-hand display

The Sofle uses ZMK v0.3 and the `nice_view_gem` shield. `CONFIG_NYFFELS_DISPLAY_ART=y`
enables this repository's Zephyr module for Sofle only. Corne and Lily58 keep their
existing configurations.

The left display is unchanged. The right display keeps GEM's battery and split
connection widgets, replacing only its decorative image: Nyffels in normal
layers, a controller whenever GAME is active. The supplied logo is preserved as
`assets/display/nyffels-logo.png`.

## Artwork

Run `python scripts/generate_display_art.py` with Pillow installed. This generates
the portrait PNG previews, controller SVG and `src/display_art_images.c`. The
source PNG is not changed. Its light facets are dithered for the monochrome panel.
The controller geometry is authored in `make_controller()`; the SVG is a generated
vector preview, not a separate input.

The panel is 160×68 in native coordinates, mounted in portrait. Artwork covers
the lower 124 portrait rows, leaving the top 36 for the existing status widgets.
The C image uses LVGL's indexed 1-bit format, including both palette entries and
byte-padded rows. Its preview is `docs/sofle-display.html`.

## Synchronization

The left half reads the actual GAME layer state (default index 3, configurable
with `CONFIG_NYFFELS_DISPLAY_GAME_LAYER`) on layer changes. It sends a tiny state
update using `zmk_split_central_invoke_behavior()` over ZMK's existing encrypted
split behavior channel. The private `nyart` behavior name fits the eight-character
BLE name limit. This behavior only sets display state; it never emits a key or
changes keyboard layers.

After a peripheral BLE connection, the current state is resent once per second
for 15 seconds to cover encryption and service discovery. Every resend reads the
current state, rather than replaying an old toggle. There is no periodic polling
after this reconnect window. On disconnection, the right half shows the logo until
resynchronized. LVGL updates run exclusively on ZMK's display work queue.

## GEM integration

On the peripheral build only, `--wrap=draw_animation` redirects GEM's decoration
creation to `__wrap_draw_animation()`. This keeps upstream status widgets and the
entire central display intact, without copying or editing the external module.
This integration is based on Mechboards GEM revision
`7794ebf` (branch `nv_gem`); if its decoration entry point changes, update the
wrapper accordingly. Link-map verification should show the wrapper and exclude
the unused crystal animation from the final peripheral image.

Build both halves with `nice_nano_v2` and their existing shield combinations.
Reflash both because the central build sends display updates and the peripheral
build receives them. Studio and its stored keymap overrides remain disabled.
No bonding settings, secret storage or credentials are changed.
