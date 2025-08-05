#include <zephyr/kernel.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>

static int usb_connection_listener(const zmk_event_t *eh) {
    struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    
    if (ev->conn_state == ZMK_USB_CONN_HID) {
        // USB conectado - enviar "USB" + Enter
        k_sleep(K_MSEC(10000)); // Esperar un poco para que se establezca la conexión
        
        // Enviar U-S-B
        zmk_hid_keyboard_press(HID_USAGE_KEY_KEYBOARD_U);
        k_sleep(K_MSEC(50));
        zmk_hid_keyboard_release(HID_USAGE_KEY_KEYBOARD_U);
        
        zmk_hid_keyboard_press(HID_USAGE_KEY_KEYBOARD_S);
        k_sleep(K_MSEC(50));
        zmk_hid_keyboard_release(HID_USAGE_KEY_KEYBOARD_S);
        
        zmk_hid_keyboard_press(HID_USAGE_KEY_KEYBOARD_B);
        k_sleep(K_MSEC(50));
        zmk_hid_keyboard_release(HID_USAGE_KEY_KEYBOARD_B);
        
        // Enter
        zmk_hid_keyboard_press(HID_USAGE_KEY_KEYBOARD_RETURN_ENTER);
        k_sleep(K_MSEC(50));
        zmk_hid_keyboard_release(HID_USAGE_KEY_KEYBOARD_RETURN_ENTER);
        
    } else if (ev->conn_state == ZMK_USB_CONN_NONE) {
        // USB desconectado - podríamos enviar algo por BLE al central
        // Pero mejor no hacer nada aquí para no interrumpir
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(usb_connection, usb_connection_listener);
ZMK_SUBSCRIPTION(usb_connection, zmk_usb_conn_state_changed);