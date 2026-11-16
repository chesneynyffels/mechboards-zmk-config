# Nyffels right-hand display

The Sofle uses ZMK v0.3 and the `nice_view_gem` shield. `CONFIG_NYFFELS_DISPLAY_ART=y`
enables this repository's Zephyr module for Sofle only. Corne and Lily58 keep their
existing configurations.

The right display keeps GEM's battery and split
connection widgets, replacing only its decorative image: Nyffels in normal
layers, a controller whenever GAME is active. The supplied logo is preserved as
`assets/display/nyffels-logo.png`.

## Left status without WPM

The Sofle central build now uses `src/display_status_left.c` instead of GEM's
WPM screen. It shows battery percentage, USB/Bluetooth profile, the current
layer in a large label, connection status and the selected Bluetooth profile. GAME
uses an inverted black panel to stand out.

The top header shows the current transport (`USB` or `BT`), while the footer
always shows the selected Bluetooth slot, for example `BT 1/5`, including when
USB is in use. The total comes from ZMK's `ZMK_BLE_PROFILE_COUNT`; slot selection
is one-based on screen. Layer changes do not hide or reset the profile indicator.

The three central-only CMake options in `build.yaml` are required:

```
-DCONFIG_NYFFELS_LEFT_STATUS=y
-DCONFIG_NICE_VIEW_WIDGET_STATUS=n
-DCONFIG_ZMK_WPM=n
```

GEM normally implies `NICE_VIEW_WIDGET_STATUS`, which selects `ZMK_WPM`.
Our Kconfig exposes a prompt for the otherwise hidden GEM symbol only when
the custom left screen is enabled. This lets the explicit `n` take effect.
The resulting central firmware must contain neither `wpm_timer` nor
`wpm_work_handler`. The peripheral retains its normal GEM widgets.

The left screen listens to layer, battery, endpoint, Bluetooth-profile and USB
events on ZMK's display work queue. It skips redraws if the visible state is
unchanged. There is no WPM calculation, keystroke counter or WPM refresh timer.
Normal display maintenance and BLE processing remain necessary. No battery-life
improvement is quantified without measurement on the actual keyboard.

The 68×160 portrait canvas is rendered clockwise into the panel's native 160×68
buffer. Both buffers are static; rendering does not allocate a new full image
for each update. `docs/sofle-left-display.html` is a schematic preview.

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
creation to `__wrap_draw_animation()`. This keeps the peripheral's upstream
status widgets intact, without copying or editing the external module. The
central build disables GEM widgets and supplies its own status-screen entry point.
This integration is based on Mechboards GEM revision
`7794ebf` (branch `nv_gem`); if its decoration entry point changes, update the
wrapper accordingly. Link-map verification should show the wrapper and exclude
the unused crystal animation from the final peripheral image.

Build both halves with `nice_nano_v2` and their existing shield combinations.
Reflash both because the central build sends display updates and the peripheral
build receives them. Studio and its stored keymap overrides remain disabled.
No bonding settings, secret storage or credentials are changed.
