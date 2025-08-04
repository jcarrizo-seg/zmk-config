#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zmk/events/usb_conn_state_changed.h>

// LED rojo del Nice!Nano v2 (pin 0.09)
static const struct gpio_dt_spec led = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin = 9,  // Pin 0.09 para el LED rojo
    .dt_flags = GPIO_ACTIVE_HIGH
};

// Función que se ejecuta cuando cambia el estado del USB
static int usb_led_listener(const zmk_event_t *eh) {
    
    // Verificar si es un evento de cambio de USB
    if (as_zmk_usb_conn_state_changed(eh)) {
        
        // Obtener la información del evento
        const struct zmk_usb_conn_state_changed *usb_ev = as_zmk_usb_conn_state_changed(eh);
        
        // Si USB está conectado, encender LED rojo; si no, apagarlo
        if (usb_ev->conn_state == ZMK_USB_CONN_POWERED) {
            gpio_pin_set_dt(&led, 1);  // Encender LED rojo
        } else {
            gpio_pin_set_dt(&led, 0);  // Apagar LED rojo
        }
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

// Inicialización del módulo
static int usb_led_init(void) {
    
    // Verificar que el GPIO esté disponible
    if (!gpio_is_ready_dt(&led)) {
        return -1;
    }
    
    // Configurar el pin como salida, inicialmente apagado
    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

// Registrar el listener con ZMK
ZMK_LISTENER(usb_led, usb_led_listener);
ZMK_SUBSCRIPTION(usb_led, zmk_usb_conn_state_changed);

// Ejecutar inicialización al arrancar
SYS_INIT(usb_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);