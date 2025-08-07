/*
 * Battery Status LEDs Module for ZMK
 * Archivo: config/boards/shields/matricialsplit/battery_status_leds.c
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zmk/battery.h>
#include <zmk/usb.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>

// ✅ CORREGIDO: Crear nuestro propio módulo de logging
LOG_MODULE_REGISTER(battery_leds, CONFIG_ZMK_LOG_LEVEL);

/*
 * CONFIGURACIÓN DEVICE TREE
 * ✅ CORREGIDO: Usar DT_PATH en lugar de DT_NODELABEL
 */
#define BATTERY_STATUS_LEDS_NODE DT_PATH(battery_status_leds)

/*
 * VALIDACIONES DE COMPILACIÓN
 * Verificar que el nodo existe y está habilitado
 */
#if !DT_NODE_HAS_STATUS(BATTERY_STATUS_LEDS_NODE, okay)
#error "battery_status_leds node is not enabled. Add 'status = \"okay\";' to your overlay."
#endif

/*
 * CONSTANTES Y DEFAULTS
 */
#define BATTERY_LOW_THRESHOLD_DEFAULT 20
#define BATTERY_FULL_THRESHOLD_DEFAULT 95

// Rangos de validación
#define BATTERY_THRESHOLD_MIN 5
#define BATTERY_THRESHOLD_MAX 100
#define BATTERY_LOW_MAX 40
#define BATTERY_FULL_MIN 60
#define MIN_THRESHOLD_GAP 10

/*
 * VALIDACIONES ESTRICTAS DE UMBRALES
 */
#if DT_NODE_HAS_PROP(BATTERY_STATUS_LEDS_NODE, low_threshold)
    #define USER_LOW_THRESHOLD DT_PROP(BATTERY_STATUS_LEDS_NODE, low_threshold)
    #if (USER_LOW_THRESHOLD < BATTERY_THRESHOLD_MIN) || (USER_LOW_THRESHOLD > BATTERY_LOW_MAX)
        #error "low-threshold debe estar entre 5% y 40%"
    #endif
#else
    #define USER_LOW_THRESHOLD BATTERY_LOW_THRESHOLD_DEFAULT
#endif

#if DT_NODE_HAS_PROP(BATTERY_STATUS_LEDS_NODE, full_threshold)
    #define USER_FULL_THRESHOLD DT_PROP(BATTERY_STATUS_LEDS_NODE, full_threshold)
    #if (USER_FULL_THRESHOLD < BATTERY_FULL_MIN) || (USER_FULL_THRESHOLD > BATTERY_THRESHOLD_MAX)
        #error "full-threshold debe estar entre 60% y 100%"
    #endif
#else
    #define USER_FULL_THRESHOLD BATTERY_FULL_THRESHOLD_DEFAULT
#endif

// Validar que full > low + gap mínimo
#if (USER_FULL_THRESHOLD < USER_LOW_THRESHOLD + MIN_THRESHOLD_GAP)
    #error "full-threshold debe ser al menos 10% mayor que low-threshold"
#endif

/*
 * CONFIGURACIÓN GPIO
 */
static const struct gpio_dt_spec red_led = 
    GPIO_DT_SPEC_GET(BATTERY_STATUS_LEDS_NODE, red_gpios);

static const struct gpio_dt_spec green_led = 
    GPIO_DT_SPEC_GET(BATTERY_STATUS_LEDS_NODE, green_gpios);

static const struct gpio_dt_spec blue_led = 
    GPIO_DT_SPEC_GET(BATTERY_STATUS_LEDS_NODE, blue_gpios);

/*
 * VARIABLES GLOBALES
 */
static bool module_initialized = false;

/*
 * FUNCIONES AUXILIARES
 */

static void set_led_state(const struct gpio_dt_spec *led_spec, bool state) {
    if (!device_is_ready(led_spec->port)) {
        LOG_ERR("TEST_LOG: LED GPIO device not ready");
        return;
    }
    
    gpio_pin_set_dt(led_spec, state);
}

static void turn_off_all_leds(void) {
    set_led_state(&red_led, false);
    set_led_state(&green_led, false);
    set_led_state(&blue_led, false);
}

static void update_battery_status_leds(void) {
    if (!module_initialized) {
        LOG_WRN("TEST_LOG: Module not initialized yet");
        return;
    }

    // Obtener estado actual
    int battery_level = zmk_battery_state_of_charge();
    bool usb_connected = zmk_usb_is_powered();
    
    LOG_INF("TEST_LOG: Battery: %d%%, USB: %s", battery_level, usb_connected ? "connected" : "disconnected");
    
    // Apagar todos primero
    turn_off_all_leds();
    
    if (battery_level == -1) {
        // Sin sensor de batería - LED verde fijo para testing
        LOG_WRN("TEST_LOG: No battery sensor detected, showing green LED");
        set_led_state(&green_led, true);
        return;
    }
    
    if (!usb_connected) {
        // Sin USB conectado
        if (battery_level < USER_LOW_THRESHOLD) {
            // Batería baja - LED azul
            LOG_WRN("TEST_LOG: Battery low (%d%%), showing blue LED", battery_level);
            set_led_state(&blue_led, true);
        } else {
            // Batería OK - LEDs apagados
            LOG_INF("TEST_LOG: Battery OK (%d%%), LEDs off", battery_level);
        }
    } else {
        // USB conectado
        if (battery_level < USER_FULL_THRESHOLD) {
            // Cargando - LED rojo fijo
            LOG_INF("TEST_LOG: Charging (%d%%), showing red LED", battery_level);
            set_led_state(&red_led, true);
        } else {
            // Cargada - LED verde fijo  
            LOG_INF("TEST_LOG: Battery charged (%d%%), showing green LED", battery_level);
            set_led_state(&green_led, true);
        }
    }
}

/*
 * EVENT HANDLERS
 */
static int battery_state_changed_handler(const zmk_event_t *eh) {
    LOG_INF("TEST_LOG: Battery state changed event received");
    update_battery_status_leds();
    return ZMK_EV_EVENT_BUBBLE;
}

static int usb_conn_state_changed_handler(const zmk_event_t *eh) {
    LOG_INF("TEST_LOG: USB connection state changed event received");
    update_battery_status_leds();
    return ZMK_EV_EVENT_BUBBLE;
}

/*
 * REGISTRAR EVENT LISTENERS
 */
ZMK_LISTENER(battery_status_leds_battery_listener, battery_state_changed_handler);
ZMK_SUBSCRIPTION(battery_status_leds_battery_listener, zmk_battery_state_changed);

ZMK_LISTENER(battery_status_leds_usb_listener, usb_conn_state_changed_handler);
ZMK_SUBSCRIPTION(battery_status_leds_usb_listener, zmk_usb_conn_state_changed);

/*
 * INICIALIZACIÓN
 */
static int battery_status_leds_init(void) {
    // ✅ LOGS MÁS VISIBLES para debug
    LOG_ERR("TEST_LOG: === BATTERY STATUS LEDS INIT START ===");
    LOG_ERR("TEST_LOG: Thresholds: low=%d%%, full=%d%%", USER_LOW_THRESHOLD, USER_FULL_THRESHOLD);
    
    // Verificar que todos los GPIO devices estén listos
    if (!device_is_ready(red_led.port)) {
        LOG_ERR("TEST_LOG: Red LED GPIO device not ready");
        return -ENODEV;
    }
    
    if (!device_is_ready(green_led.port)) {
        LOG_ERR("TEST_LOG: Green LED GPIO device not ready");
        return -ENODEV;
    }
    
    if (!device_is_ready(blue_led.port)) {
        LOG_ERR("TEST_LOG: Blue LED GPIO device not ready");
        return -ENODEV;
    }
    
    LOG_ERR("TEST_LOG: All GPIO devices are ready");
    
    // Configurar pines como salida
    int ret;
    
    ret = gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("TEST_LOG: Failed to configure red LED pin: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("TEST_LOG: Failed to configure green LED pin: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("TEST_LOG: Failed to configure blue LED pin: %d", ret);
        return ret;
    }
    
    LOG_ERR("TEST_LOG: All GPIO pins configured successfully");
    
    // Marcar como inicializado
    module_initialized = true;
    
    // Actualizar estado inicial
    update_battery_status_leds();
    
    LOG_ERR("TEST_LOG: === BATTERY STATUS LEDS INIT COMPLETE ===");
    return 0;
}

/*
 * ✅ CORREGIDO: Cambiar de POST_KERNEL a APPLICATION
 * para asegurar que ZMK esté completamente inicializado
 */
SYS_INIT(battery_status_leds_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);