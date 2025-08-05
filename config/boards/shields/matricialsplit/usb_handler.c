#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>

// Solo compilar el código si el dispositivo tiene definido el alias usb-status-led
#if DT_HAS_ALIAS(usb_status_led)
    #define USB_LED_NODE DT_ALIAS(usb_status_led)
    static const struct gpio_dt_spec usb_led = GPIO_DT_SPEC_GET(USB_LED_NODE, gpios);
#endif

static int usb_connection_listener(const zmk_event_t *eh) {
#if DT_HAS_ALIAS(usb_status_led)
    // Solo ejecutar si este dispositivo tiene el alias definido
    struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    
    if (ev->conn_state == ZMK_USB_CONN_HID) {
        // USB conectado como teclado - encender LED
        gpio_pin_set_dt(&usb_led, 1);
        k_sleep(K_MSEC(10000));
        gpio_pin_set_dt(&usb_led, 0);
        k_sleep(K_MSEC(10000));
        gpio_pin_set_dt(&usb_led, 1);
        
    } else if (ev->conn_state == ZMK_USB_CONN_NONE) {
        // USB desconectado - apagar LED
        gpio_pin_set_dt(&usb_led, 0);
    }
#endif
    
    return ZMK_EV_EVENT_BUBBLE;
}

// Inicializar LED solo si está definido
static int init_usb_handler(void) {
#if DT_HAS_ALIAS(usb_status_led)
    if (!gpio_is_ready_dt(&usb_led)) {
        return -ENODEV;
    }
    
    return gpio_pin_configure_dt(&usb_led, GPIO_OUTPUT_INACTIVE);
#else
    // Si no hay LED definido, no hacer nada
    return 0;
#endif
}

ZMK_LISTENER(usb_connection, usb_connection_listener);
ZMK_SUBSCRIPTION(usb_connection, zmk_usb_conn_state_changed);
SYS_INIT(init_usb_handler, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);