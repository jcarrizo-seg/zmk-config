/*
 * Battery Status LEDs Module for ZMK
 * Archivo: config/boards/shields/mi_teclado/battery_status_leds.c
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

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/*
 * CONFIGURACIÓN DEVICE TREE
 * Obtener referencia al nodo usando el label definido en el overlay
 */
#define BATTERY_STATUS_LEDS_NODE DT_NODELABEL(battery_status_leds)

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
 * Estas macros causan errores de compilación si los valores son inválidos
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
 * Convertir propiedades device tree a estructuras gpio_dt_spec
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

/**
 * Configurar estado de un LED
 * @param led_spec: Especificación GPIO del LED
 * @param state: true = encender, false = apagar
 */
static void set_led_state(const struct gpio_dt_spec *led_spec, bool state) {
    if (!device_is_ready(led_spec->port)) {
        LOG_ERR("LED GPIO device not ready");
        return;
    }
    
    gpio_pin_set_dt(led_spec, state);
}

/**
 * Apagar todos los LEDs
 */
static void turn_off_all_leds(void) {
    set_led_state(&red_led, false);
    set_led_state(&green_led, false);
    set_led_state(&blue_led, false);
}

/**
 * Actualizar estado de LEDs según batería y USB
 */
static void update_battery_status_leds(void) {
    if (!module_initialized) {
        return;
    }

    // Obtener estado actual
    int battery_level = zmk_battery_state_of_charge();
    bool usb_connected = zmk_usb_is_powered();
    
    LOG_INF("Battery: %d%%, USB: %s", battery_level, usb_connected ? "connected" : "disconnected");
    
    // Apagar todos primero
    turn_off_all_leds();
    
    /*
     * LÓGICA DE ESTADOS:
     * 
     * | Condición           | Batería | USB | R | G | B |
     * |--------------------|---------|----- |---|---|---|
     * | Sin sensor batería | N/A     | any | - | ✓ | - |
     * | Batería baja       | < 20%   | no  | - | - | ✓ |
     * | Batería OK         | ≥ 20%   | no  | - | - | - |
     * | Cargando           | < 95%   | sí  | ✓ | - | - |
     * | Cargada            | ≥ 95%   | sí  | - | ✓ | - |
     */
    
    if (battery_level == -1) {
        // Sin sensor de batería - LED verde fijo para testing
        LOG_WRN("No battery sensor detected, showing green LED");
        set_led_state(&green_led, true);
        return;
    }
    
    if (!usb_connected) {
        // Sin USB conectado
        if (battery_level < USER_LOW_THRESHOLD) {
            // Batería baja - LED azul
            LOG_WRN("Battery low (%d%%), showing blue LED", battery_level);
            set_led_state(&blue_led, true);
        } else {
            // Batería OK - LEDs apagados
            LOG_INF("Battery OK (%d%%), LEDs off", battery_level);
            // Ya están apagados
        }
    } else {
        // USB conectado
        if (battery_level < USER_FULL_THRESHOLD) {
            // Cargando - LED rojo fijo
            LOG_INF("Charging (%d%%), showing red LED", battery_level);
            set_led_state(&red_led, true);
        } else {
            // Cargada - LED verde fijo  
            LOG_INF("Battery charged (%d%%), showing green LED", battery_level);
            set_led_state(&green_led, true);
        }
    }
}

/*
 * EVENT HANDLERS
 */

/**
 * Handler para cambios de estado de batería
 */
static int battery_state_changed_handler(const zmk_event_t *eh) {
    LOG_INF("Battery state changed event received");
    update_battery_status_leds();
    return ZMK_EV_EVENT_BUBBLE;
}

/**
 * Handler para cambios de conexión USB
 */
static int usb_conn_state_changed_handler(const zmk_event_t *eh) {
    LOG_INF("USB connection state changed event received");
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
    LOG_INF("Initializing Battery Status LEDs");
    LOG_INF("Thresholds: low=%d%%, full=%d%%", USER_LOW_THRESHOLD, USER_FULL_THRESHOLD);
    
    // Verificar que todos los GPIO devices estén listos
    if (!device_is_ready(red_led.port)) {
        LOG_ERR("Red LED GPIO device not ready");
        return -ENODEV;
    }
    
    if (!device_is_ready(green_led.port)) {
        LOG_ERR("Green LED GPIO device not ready");
        return -ENODEV;
    }
    
    if (!device_is_ready(blue_led.port)) {
        LOG_ERR("Blue LED GPIO device not ready");
        return -ENODEV;
    }
    
    // Configurar pines como salida
    int ret;
    
    ret = gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure red LED pin: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure green LED pin: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure blue LED pin: %d", ret);
        return ret;
    }
    
    // Marcar como inicializado
    module_initialized = true;
    
    // Actualizar estado inicial
    update_battery_status_leds();
    
    LOG_INF("Battery Status LEDs initialized successfully");
    return 0;
}

/*
 * SISTEMA DE INICIALIZACIÓN DE ZEPHYR
 * POST_KERNEL: Se ejecuta después de que el kernel esté listo
 * CONFIG_APPLICATION_INIT_PRIORITY: Prioridad de inicialización
 */
SYS_INIT(battery_status_leds_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);