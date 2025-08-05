#include <zephyr/kernel.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/event_manager.h>
#include <dt-bindings/zmk/keys.h>

// Work queue para enviar teclas de manera asíncrona
static struct k_work_delayable usb_connected_work;

// Función para enviar una tecla
static void send_keypress(uint32_t keycode) {
    // Crear y enviar evento de press
    struct zmk_keycode_state_changed *press_ev = new_zmk_keycode_state_changed();
    if (press_ev != NULL) {
        press_ev->usage_id = keycode;
        press_ev->state = true;
        press_ev->timestamp = k_uptime_get();
        ZMK_EVENT_RAISE(press_ev);
    }
    
    k_sleep(K_MSEC(50));
    
    // Crear y enviar evento de release
    struct zmk_keycode_state_changed *release_ev = new_zmk_keycode_state_changed();
    if (release_ev != NULL) {
        release_ev->usage_id = keycode;
        release_ev->state = false;
        release_ev->timestamp = k_uptime_get();
        ZMK_EVENT_RAISE(release_ev);
    }
    
    k_sleep(K_MSEC(50));
}

// Variable global para indicar estado USB
static bool usb_is_connected = false;

static void send_usb_indicator(struct k_work *work) {
    // Esperar a que USB esté completamente establecido
    k_sleep(K_MSEC(2000));
    
    // Enviar "USB" + Enter
    send_keypress(U);  // U
    send_keypress(S);  // S  
    send_keypress(B);  // B
    send_keypress(RET); // Enter
    
    usb_is_connected = true;
}

static int usb_connection_listener(const zmk_event_t *eh) {
    struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    
    if (ev->conn_state == ZMK_USB_CONN_HID) {
        // USB conectado - programar indicador
        k_work_schedule(&usb_connected_work, K_MSEC(100));
        
    } else if (ev->conn_state == ZMK_USB_CONN_NONE) {
        // USB desconectado - actualizar estado
        usb_is_connected = false;
        k_work_cancel_delayable(&usb_connected_work);
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

// Función para que otros módulos puedan consultar el estado USB
bool is_usb_connected(void) {
    return usb_is_connected;
}

// Inicializar el work queue
static int init_usb_handler(void) {
    k_work_init_delayable(&usb_connected_work, send_usb_indicator);
    return 0;
}

ZMK_LISTENER(usb_connection, usb_connection_listener);
ZMK_SUBSCRIPTION(usb_connection, zmk_usb_conn_state_changed);
SYS_INIT(init_usb_handler, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);