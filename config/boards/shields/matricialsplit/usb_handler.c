#include <zephyr/kernel.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>

static void send_key(uint32_t usage_id) {
    // Crear evento de key press
    struct zmk_keycode_state_changed *press_event = 
        new_zmk_keycode_state_changed();
    press_event->usage_id = usage_id;
    press_event->state = true;
    ZMK_EVENT_RAISE(press_event);
    
    k_sleep(K_MSEC(50));
    
    // Crear evento de key release  
    struct zmk_keycode_state_changed *release_event = 
        new_zmk_keycode_state_changed();
    release_event->usage_id = usage_id;
    release_event->state = false;
    ZMK_EVENT_RAISE(release_event);
    
    k_sleep(K_MSEC(50));
}

static void send_usb_text(struct k_work *work) {
    k_sleep(K_MSEC(2000)); // Dar tiempo para que se establezca USB
    
    send_key(HID_USAGE_KEY_KEYBOARD_U);
    send_key(HID_USAGE_KEY_KEYBOARD_S); 
    send_key(HID_USAGE_KEY_KEYBOARD_B);
    send_key(HID_USAGE_KEY_KEYBOARD_RETURN_ENTER);
}

static struct k_work_delayable usb_work;

static int usb_connection_listener(const zmk_event_t *eh) {
    struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    
    if (ev->conn_state == ZMK_USB_CONN_HID) {
        k_work_schedule(&usb_work, K_MSEC(100));
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

static int init_usb_handler(void) {
    k_work_init_delayable(&usb_work, send_usb_text);
    return 0;
}

ZMK_LISTENER(usb_connection, usb_connection_listener);
ZMK_SUBSCRIPTION(usb_connection, zmk_usb_conn_state_changed);
SYS_INIT(init_usb_handler, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);