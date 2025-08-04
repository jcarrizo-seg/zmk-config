#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zmk/events/usb_conn_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Probar múltiples pines posibles para el LED rojo
static const struct gpio_dt_spec led_candidates[] = {
    {.port = DEVICE_DT_GET(DT_NODELABEL(gpio0)), .pin = 9, .dt_flags = GPIO_ACTIVE_HIGH},   // Pin más común
    {.port = DEVICE_DT_GET(DT_NODELABEL(gpio0)), .pin = 10, .dt_flags = GPIO_ACTIVE_HIGH},  // Alternativo
    {.port = DEVICE_DT_GET(DT_NODELABEL(gpio1)), .pin = 9, .dt_flags = GPIO_ACTIVE_HIGH},   // Puerto 1
};

static const struct gpio_dt_spec *active_led = NULL;
static int num_candidates = sizeof(led_candidates) / sizeof(led_candidates[0]);

static int usb_led_listener(const zmk_event_t *eh) {
    if (as_zmk_usb_conn_state_changed(eh)) {
        const struct zmk_usb_conn_state_changed *usb_ev = as_zmk_usb_conn_state_changed(eh);
        
        LOG_INF("USB state changed: %d", usb_ev->conn_state);
        
        if (active_led != NULL) {
            if (usb_ev->conn_state == ZMK_USB_CONN_POWERED) {
                LOG_INF("USB connected - turning LED ON");
                gpio_pin_set_dt(active_led, 1);
            } else {
                LOG_INF("USB disconnected - turning LED OFF");
                gpio_pin_set_dt(active_led, 0);
            }
        } else {
            LOG_ERR("No active LED configured");
        }
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

static int usb_led_init(void) {
    LOG_INF("Initializing USB LED module");
    
    // Probar cada pin candidato
    for (int i = 0; i < num_candidates; i++) {
        if (gpio_is_ready_dt(&led_candidates[i])) {
            int ret = gpio_pin_configure_dt(&led_candidates[i], GPIO_OUTPUT_INACTIVE);
            if (ret == 0) {
                active_led = &led_candidates[i];
                LOG_INF("Successfully configured LED on port %p, pin %d", 
                       active_led->port, active_led->pin);
                
                // Probar encender/apagar para verificar que funciona
                gpio_pin_set_dt(active_led, 1);
                k_msleep(100);
                gpio_pin_set_dt(active_led, 0);
                
                return 0;
            } else {
                LOG_WRN("Failed to configure pin %d on port %p: %d", 
                       led_candidates[i].pin, led_candidates[i].port, ret);
            }
        } else {
            LOG_WRN("GPIO port %p not ready for pin %d", 
                   led_candidates[i].port, led_candidates[i].pin);
        }
    }
    
    LOG_ERR("Could not configure any LED pin");
    return -1;
}

ZMK_LISTENER(usb_led, usb_led_listener);
ZMK_SUBSCRIPTION(usb_led, zmk_usb_conn_state_changed);
SYS_INIT(usb_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);