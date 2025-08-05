#include <zephyr/kernel.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>

// Sin logs por ahora - usaremos indicadores visuales

// Work queue para enviar teclas de manera asíncrona
static struct k_work_delayable usb_connected_work;

static void send_usb_indicator(struct k_work *work) {
    // Esperar a que USB esté completamente establecido
    k_sleep(K_MSEC(2000));
    
    // Por ahora, solo una pausa para confirmar que se ejecuta
    // TODO: Implementar envío de teclas o LEDs
}

static int usb_connection_listener(const zmk_event_t *eh) {
    struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    
    if (ev->conn_state == ZMK_USB_CONN_HID) {
        // USB conectado - programar indicador
        k_work_schedule(&usb_connected_work, K_MSEC(100));
        
    } else if (ev->conn_state == ZMK_USB_CONN_NONE) {
        // USB desconectado - cancelar trabajo pendiente
        k_work_cancel_delayable(&usb_connected_work);
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

// Inicializar el work queue
static int init_usb_handler(void) {
    k_work_init_delayable(&usb_connected_work, send_usb_indicator);
    return 0;
}

ZMK_LISTENER(usb_connection, usb_connection_listener);
ZMK_SUBSCRIPTION(usb_connection, zmk_usb_conn_state_changed);
SYS_INIT(init_usb_handler, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);