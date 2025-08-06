#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>

// LED rojo en P0.15
#define LED_RED_NODE DT_NODELABEL(gpio0)
static const struct gpio_dt_spec red_led = {
    .port = DEVICE_DT_GET(LED_RED_NODE),
    .pin = 15,
    .dt_flags = GPIO_ACTIVE_HIGH
};

static int usb_connection_listener(const zmk_event_t *eh) {
    struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    
    if (ev->conn_state == ZMK_USB_CONN_HID) {
        // USB conectado - encender LED
        gpio_pin_set_dt(&red_led, 1);
        k_sleep(K_MSEC(2000));
        gpio_pin_set_dt(&red_led, 0);
        k_sleep(K_MSEC(2000));
        gpio_pin_set_dt(&red_led, 1);
        k_sleep(K_MSEC(2000));
        gpio_pin_set_dt(&red_led, 0);
        
    } else if (ev->conn_state == ZMK_USB_CONN_NONE) {
        // USB desconectado - apagar LED
        gpio_pin_set_dt(&red_led, 0);
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

// Inicializar LED
static int init_usb_handler(void) {
    if (!gpio_is_ready_dt(&red_led)) {
        return -ENODEV;
    }
    
    return gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
}

ZMK_LISTENER(usb_connection, usb_connection_listener);
ZMK_SUBSCRIPTION(usb_connection, zmk_usb_conn_state_changed);
SYS_INIT(init_usb_handler, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);