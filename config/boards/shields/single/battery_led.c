/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/event_manager.h>

// Crear nuestro propio módulo de logging
LOG_MODULE_REGISTER(battery_led, CONFIG_ZMK_LOG_LEVEL);

// Definir los pines de los LEDs según tu configuración
#define LED_GREEN_NODE  DT_ALIAS(led_green)
#define LED_RED_NODE    DT_ALIAS(led_red)
#define LED_YELLOW_NODE DT_ALIAS(led_yellow)
#define LED_BLUE_NODE   DT_ALIAS(led_blue)

static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LED_GREEN_NODE, gpios);
static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(LED_RED_NODE, gpios);
static const struct gpio_dt_spec led_yellow = GPIO_DT_SPEC_GET(LED_YELLOW_NODE, gpios);
static const struct gpio_dt_spec led_blue = GPIO_DT_SPEC_GET(LED_BLUE_NODE, gpios);

// Función para actualizar los LEDs según el porcentaje de batería
static void update_battery_leds(uint8_t battery_level) {
    LOG_INF("*** BATTERY LED UPDATE ***");
    LOG_INF("Battery level: %d%%", battery_level);
    
    // Apagar todos los LEDs primero
    gpio_pin_set_dt(&led_green, 0);
    gpio_pin_set_dt(&led_red, 0);
    gpio_pin_set_dt(&led_yellow, 0);
    gpio_pin_set_dt(&led_blue, 0);
    
    if (battery_level > 80) {
        // Verde: batería muy alta (>80%)
        gpio_pin_set_dt(&led_green, 1);
        LOG_INF("LED Status: GREEN (Very High - %d%%)", battery_level);
    } else if (battery_level > 60) {
        // Azul: batería alta (60-80%)
        gpio_pin_set_dt(&led_blue, 1);
        LOG_INF("LED Status: BLUE (High - %d%%)", battery_level);
    } else if (battery_level > 30) {
        // Amarillo: batería media (30-60%)
        gpio_pin_set_dt(&led_yellow, 1);
        LOG_INF("LED Status: YELLOW (Medium - %d%%)", battery_level);
    } else {
        // Rojo: batería baja (<30%)
        gpio_pin_set_dt(&led_red, 1);
        LOG_INF("LED Status: RED (Low - %d%%)", battery_level);
    }
    LOG_INF("*** END BATTERY UPDATE ***");
}

// Listener para eventos de cambio de batería
static int battery_state_listener(const zmk_event_t *eh) {
    LOG_INF("*** BATTERY EVENT RECEIVED ***");
    
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        LOG_WRN("Battery event is NULL!");
        return ZMK_EV_EVENT_BUBBLE;
    }
    
    LOG_INF("Event battery level: %d%%", ev->state_of_charge);
    update_battery_leds(ev->state_of_charge);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(battery_led_listener, battery_state_listener);
ZMK_SUBSCRIPTION(battery_led_listener, zmk_battery_state_changed);

// Work handler para updates periódicos (para testing)
static void battery_check_work_handler(struct k_work *work) {
    uint8_t current_level = zmk_battery_state_of_charge();
    LOG_INF("*** PERIODIC BATTERY CHECK ***");
    LOG_INF("Current battery level: %d%%", current_level);
    update_battery_leds(current_level);
}

static K_WORK_DEFINE(battery_check_work, battery_check_work_handler);

// Timer para checks periódicos (solo para debug)
static void battery_check_timer_handler(struct k_timer *timer) {
    k_work_submit(&battery_check_work);
}

static K_TIMER_DEFINE(battery_check_timer, battery_check_timer_handler, NULL);

// Inicialización del módulo
static int battery_led_init(void) {
    int ret;
    
    LOG_INF("*** BATTERY LED INIT START ***");
    
    // Verificar y configurar LEDs
    if (!gpio_is_ready_dt(&led_green)) {
        LOG_ERR("Green LED device not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&led_red)) {
        LOG_ERR("Red LED device not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&led_yellow)) {
        LOG_ERR("Yellow LED device not ready");
        return -ENODEV;
    }
    if (!gpio_is_ready_dt(&led_blue)) {
        LOG_ERR("Blue LED device not ready");
        return -ENODEV;
    }
    
    LOG_INF("All LED devices are ready");
    
    // Configurar LEDs como outputs
    ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure green LED: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure red LED: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&led_yellow, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure yellow LED: %d", ret);
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure blue LED: %d", ret);
        return ret;
    }
    
    LOG_INF("All LEDs configured successfully");
    
    // Obtener el nivel inicial de batería
    uint8_t initial_level = zmk_battery_state_of_charge();
    
    LOG_INF("*** BATTERY LED MODULE STARTED ***");
    LOG_INF("Initial battery level: %d%%", initial_level);
    LOG_INF("LEDs configured on pins: G=10, R=16, Y=14, B=15");
    LOG_INF("Battery reporting interval: %d seconds", CONFIG_ZMK_BATTERY_REPORT_INTERVAL);
    LOG_INF("*** INITIALIZATION COMPLETE ***");
    
    // Mostrar LEDs iniciales
    update_battery_leds(initial_level);
    
    // Iniciar timer para checks periódicos (cada 30 segundos para debug)
    k_timer_start(&battery_check_timer, K_SECONDS(10), K_SECONDS(30));
    
    LOG_INF("Battery LED module initialized successfully");
    return 0;
}

SYS_INIT(battery_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);