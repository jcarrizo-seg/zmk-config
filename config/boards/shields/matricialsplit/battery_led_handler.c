#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>

LOG_MODULE_REGISTER(battery_led, CONFIG_ZMK_LOG_LEVEL);

// Solo compilar si la funcionalidad está habilitada
#ifdef CONFIG_BATTERY_LED_INDICATOR

// Definir los nodos de los LEDs
#define RED_LED_NODE   DT_ALIAS(battery_red_led)
#define GREEN_LED_NODE DT_ALIAS(battery_green_led)  
#define YELLOW_LED_NODE  DT_ALIAS(battery_yellow_led)

// Verificar que los LEDs estén definidos
#if DT_NODE_EXISTS(RED_LED_NODE) && DT_NODE_EXISTS(GREEN_LED_NODE) && DT_NODE_EXISTS(YELLOW_LED_NODE)
    static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(RED_LED_NODE, gpios);
    static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(GREEN_LED_NODE, gpios);
    static const struct gpio_dt_spec yellow_led = GPIO_DT_SPEC_GET(YELLOW_LED_NODE, gpios);
    #define HAS_BATTERY_LEDS 1
#else
    #define HAS_BATTERY_LEDS 0
#endif

#if HAS_BATTERY_LEDS

// Estados del sistema
static bool usb_connected = false;
static uint8_t battery_level = 100;  // Asumir batería llena al inicio

// Función para apagar todos los LEDs
static void turn_off_all_leds(void) {
    gpio_pin_set_dt(&red_led, 0);
    gpio_pin_set_dt(&green_led, 0);
    gpio_pin_set_dt(&yellow_led, 0);
}

// Función para actualizar el estado de los LEDs
static void update_battery_leds(void) {
    // Primero apagar todos los LEDs
    turn_off_all_leds();

    gpio_pin_set_dt(&yellow_led, 1);
    k_sleep(K_MSEC(2000));
    gpio_pin_set_dt(&yellow_led, 0);
    k_sleep(K_MSEC(2000));
    
    if (usb_connected) {
        // USB conectado
        if (battery_level >= CONFIG_BATTERY_FULL_THRESHOLD) {
            // Estado 4: USB conectado + batería >= umbral_superior → LED verde
            gpio_pin_set_dt(&green_led, 1);
            
            #ifdef CONFIG_BATTERY_LED_DEBUG
            LOG_INF("Battery LED: Green (USB connected, battery full: %d%%)", battery_level);
            #endif
        } else {
            // Estado 3: USB conectado + batería < umbral_superior → LED rojo
            gpio_pin_set_dt(&red_led, 1);
            
            #ifdef CONFIG_BATTERY_LED_DEBUG
            LOG_INF("Battery LED: Red (USB connected, charging: %d%%)", battery_level);
            #endif
        }
    } else {
        // USB desconectado
        if (battery_level < CONFIG_BATTERY_LOW_THRESHOLD) {
            // Estado 2: USB desconectado + batería < umbral_inferior → LED azul
            gpio_pin_set_dt(&yellow_led, 1);
            
            #ifdef CONFIG_BATTERY_LED_DEBUG
            LOG_INF("Battery LED: Blue (USB disconnected, low battery: %d%%)", battery_level);
            #endif
        }
        // Estado 1: USB desconectado + batería > umbral_inferior → Ningún LED
        // (ya están apagados por turn_off_all_leds())
        
        #ifdef CONFIG_BATTERY_LED_DEBUG
        else {
            LOG_INF("Battery LED: Off (USB disconnected, battery OK: %d%%)", battery_level);
        }
        #endif
    }
}

// Listener para cambios en el estado de la batería
static int battery_state_listener(const zmk_event_t *eh) {
    struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    
    battery_level = ev->state_of_charge;
    
    #ifdef CONFIG_BATTERY_LED_DEBUG
    LOG_INF("Battery level changed: %d%%", battery_level);
    #endif
    
    update_battery_leds();
    
    return ZMK_EV_EVENT_BUBBLE;
}

// Listener para cambios en el estado USB
static int usb_conn_state_listener(const zmk_event_t *eh) {
    struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    
    bool new_usb_state = (ev->conn_state != ZMK_USB_CONN_NONE);
    
    if (new_usb_state != usb_connected) {
        usb_connected = new_usb_state;
        
        #ifdef CONFIG_BATTERY_LED_DEBUG
        LOG_INF("USB connection changed: %s", usb_connected ? "connected" : "disconnected");
        #endif
        
        update_battery_leds();
    }
    
    return ZMK_EV_EVENT_BUBBLE;
}

// Inicialización de los LEDs
static int init_battery_leds(void) {
    // Verificar que todos los GPIOs estén listos
    if (!gpio_is_ready_dt(&red_led) || 
        !gpio_is_ready_dt(&green_led) || 
        !gpio_is_ready_dt(&yellow_led)) {
        LOG_ERR("Battery LEDs GPIO not ready");
        return -ENODEV;
    }
    
    // Configurar los pines como salida inactiva
    int ret = gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure red LED GPIO: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure green LED GPIO: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&yellow_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure blue LED GPIO: %d", ret);
        return ret;
    }
    
    // Apagar todos los LEDs al inicio
    turn_off_all_leds();
    
    LOG_INF("Battery LED indicator initialized (Low: %d%%, Full: %d%%)", 
            CONFIG_BATTERY_LOW_THRESHOLD, CONFIG_BATTERY_FULL_THRESHOLD);
    
    return 0;
}

// Registrar listeners y inicialización
ZMK_LISTENER(battery_led_battery, battery_state_listener);
ZMK_SUBSCRIPTION(battery_led_battery, zmk_battery_state_changed);

ZMK_LISTENER(battery_led_usb, usb_conn_state_listener);
ZMK_SUBSCRIPTION(battery_led_usb, zmk_usb_conn_state_changed);

SYS_INIT(init_battery_leds, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif // HAS_BATTERY_LEDS

#endif // CONFIG_BATTERY_LED_INDICATOR