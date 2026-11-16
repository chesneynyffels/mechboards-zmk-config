/* SPDX-License-Identifier: MIT */
#include <zephyr/bluetooth/conn.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/split/central.h>

/* Reuse ZMK's existing encrypted split behavior transport. No new BLE service,
 * pairing, bond storage, host application or keypress is involved. */
#define ART_BEHAVIOR_NAME DEVICE_DT_NAME(DT_NODELABEL(nyart))
BUILD_ASSERT(sizeof(ART_BEHAVIOR_NAME) <= 9, "Split BLE behavior names must fit in 8 characters");

static atomic_t reconnect_remaining;

static void send_current_art(void) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = ART_BEHAVIOR_NAME,
        .param1 = zmk_keymap_layer_active(CONFIG_NYFFELS_DISPLAY_GAME_LAYER),
    };
    struct zmk_behavior_binding_event event = {.timestamp = k_uptime_get()};
    /* Sofle has one peripheral, at split source index zero. Only send state;
     * the receiving behavior never activates layers or emits host input. */
    zmk_split_central_invoke_behavior(0, &binding, event, true);
}

static void layer_sync_work_cb(struct k_work *work) { send_current_art(); }
K_WORK_DEFINE(layer_sync_work, layer_sync_work_cb);

static int art_layer_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev && ev->layer == CONFIG_NYFFELS_DISPLAY_GAME_LAYER) {
        /* Read the final layer state in the work callback, including &to BASE. */
        k_work_submit(&layer_sync_work);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(nyffels_art_sync, art_layer_listener);
ZMK_SUBSCRIPTION(nyffels_art_sync, zmk_layer_state_changed);

static void reconnect_sync_work_cb(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(reconnect_sync_work, reconnect_sync_work_cb);

static void reconnect_sync_work_cb(struct k_work *work) {
    if (atomic_get(&reconnect_remaining) <= 0) {
        return;
    }
    send_current_art();
    /* Split service discovery happens after the BLE connection callback. Retry
     * for 15 seconds so a restarted right half also receives the current state.
     * No periodic polling once this short reconnect window has finished. */
    if (atomic_dec(&reconnect_remaining) > 1) {
        k_work_reschedule(&reconnect_sync_work, K_SECONDS(1));
    }
}

static bool is_split_connection(struct bt_conn *conn) {
    struct bt_conn_info info;
    return bt_conn_get_info(conn, &info) == 0 && info.role == BT_CONN_ROLE_CENTRAL;
}

static void art_connected(struct bt_conn *conn, uint8_t err) {
    if (!err && is_split_connection(conn)) {
        atomic_set(&reconnect_remaining, 15);
        k_work_reschedule(&reconnect_sync_work, K_SECONDS(1));
    }
}

static void art_disconnected(struct bt_conn *conn, uint8_t reason) {
    if (is_split_connection(conn)) {
        atomic_clear(&reconnect_remaining);
        k_work_cancel_delayable(&reconnect_sync_work);
    }
}

BT_CONN_CB_DEFINE(nyffels_art_connections) = {
    .connected = art_connected,
    .disconnected = art_disconnected,
};
