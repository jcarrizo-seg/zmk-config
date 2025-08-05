#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>

// Definir el LED azul en P0.15
#define LED_BLUE_NODE DT_NODELABEL(gpio0)
static const struct gpio_dt_spec blue_led = {
    .port = DEVICE_DT_GET(LED_BLUE_NODE),
    .pin = 15,
    .dt_flags = GPIO_ACTIVE_LOW
};

// Work queue para enviar teclas de manera asíncrona
static struct k_work_delayable usb_connected_work;

// Work queue para enviar indicador USB
static struct k_work_delayable usb_connected_work;

// Variable global para indicar estado USB
static bool usb_is_connected = false;

static void send_usb_indicator(struct k_work *work) {
    // Esperar a que USB esté completamente establecido
    k_sleep(K_MSEC(2000));
    
    // Encender LED azul por 5 segundos
    gpio_pin_set_dt(&blue_led, 1);
    k_sleep(K_MSEC(10000));
    gpio_pin_set_dt(&blue_led, 0);
    
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

// Inicializar el LED al arrancar
static int init_usb_handler(void) {
    // Inicializar work queue
    k_work_init_delayable(&usb_connected_work, send_usb_indicator);
    
    // Configurar LED como salida
    if (!gpio_is_ready_dt(&blue_led)) {
        return -ENODEV;
    }
    
    int ret = gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

ZMK_LISTENER(usb_connection, usb_connection_listener);
ZMK_SUBSCRIPTION(usb_connection, zmk_usb_conn_state_changed);
SYS_INIT(init_usb_handler, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);