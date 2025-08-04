#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zmk/events/usb_conn_state_changed.h>

// LED azul del Nice!Nano v2 (pin 0.06)
static const struct gpio_dt_spec led = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin = 6,
    .dt_flags = GPIO_ACTIVE_HIGH
};

static bool led_initialized = false;

static int usb_led_listener(const zmk_event_t *eh) {
    if (!led_initialized) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    
    if (as_zmk_usb_conn_state_changed(eh)) {
        const struct zmk_usb_conn_state_changed *usb_ev = as_zmk_usb_conn_state_changed(eh);
        
        if (usb_ev->conn_state == ZMK_USB_CONN_POWERED) {
            gpio_pin_set_dt(&led, 1);  // Encender LED azul
        } else {
            gpio_pin_set_dt(&led, 0);  // Apagar LED azul
        }
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

static int usb_led_init(void) {
    if (!gpio_is_ready_dt(&led)) {
        return -ENODEV;
    }
    
    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        return ret;
    }
    
    led_initialized = true;
    
    // Test rápido: parpadear 3 veces al iniciar para confirmar que funciona
    for (int i = 0; i < 20; i++) {
        gpio_pin_set_dt(&led, 1);
        k_msleep(300);
        gpio_pin_set_dt(&led, 0);
        k_msleep(300);
    }
    
    return 0;
}

ZMK_LISTENER(usb_led, usb_led_listener);
ZMK_SUBSCRIPTION(usb_led, zmk_usb_conn_state_changed);
SYS_INIT(usb_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);