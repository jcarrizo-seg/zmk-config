#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Work queue para enviar teclas de manera asíncrona
static struct k_work_delayable usb_connected_work;

static void send_usb_indicator(struct k_work *work) {
    // Esperar a que USB esté completamente establecido
    k_sleep(K_MSEC(2000));
    
    // Por ahora solo dejamos el log para confirmar que funciona
    // TODO: Implementar envío de teclas cuando tengamos la API correcta
    LOG_INF("¡USB conectado! Aquí enviaríamos las teclas USB");
}

static int usb_connection_listener(const zmk_event_t *eh) {
    struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    
    if (ev->conn_state == ZMK_USB_CONN_HID) {
        LOG_INF("USB conectado - enviando indicador");
        // Programar el envío de teclas
        k_work_schedule(&usb_connected_work, K_MSEC(100));
        
    } else if (ev->conn_state == ZMK_USB_CONN_NONE) {
        LOG_INF("USB desconectado - funcionando con batería");
        // Cancelar trabajo pendiente si existe
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