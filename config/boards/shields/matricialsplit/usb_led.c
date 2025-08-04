#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zmk/events/usb_conn_state_changed.h>

// LED rojo del Nice!Nano v2 (pin 0.09)
static const struct gpio_dt_spec led = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin = 9,
    .dt_flags = GPIO_ACTIVE_HIGH
};

static int usb_led_listener(const zmk_event_t *eh) {
    if (as_zmk_usb_conn_state_changed(eh)) {
        const struct zmk_usb_conn_state_changed *usb_ev = as_zmk_usb_conn_state_changed(eh);
        
        if (usb_ev->conn_state == ZMK_USB_CONN_POWERED) {
            gpio_pin_set_dt(&led, 1);  // Encender LED rojo
        } else {
            gpio_pin_set_dt(&led, 0);  // Apagar LED rojo
        }
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

static int usb_led_init(void) {
    if (!gpio_is_ready_dt(&led)) {
        return -1;
    }
    
    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

ZMK_LISTENER(usb_led, usb_led_listener);
ZMK_SUBSCRIPTION(usb_led, zmk_usb_conn_state_changed);
SYS_INIT(usb_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);