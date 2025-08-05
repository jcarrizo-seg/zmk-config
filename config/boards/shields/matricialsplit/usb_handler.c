#include <zephyr/kernel.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>

// Work queue para enviar teclas de manera asíncrona
static struct k_work_delayable usb_connected_work;

// Work queue para enviar indicador USB
static struct k_work_delayable usb_connected_work;

// Variable global para indicar estado USB
static bool usb_is_connected = false;

static void send_usb_indicator(struct k_work *work) {
    // Esperar a que USB esté completamente establecido
    k_sleep(K_MSEC(2000));
    
    // Crear un delay artificial que puedas notar
    // Si funciona, el teclado se sentirá "lento" por 10 segundos
    k_sleep(K_MSEC(10000));
    
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