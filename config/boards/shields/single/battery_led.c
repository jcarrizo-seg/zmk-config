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

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

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
    // Obtener el voltaje de la batería para más información
    uint16_t battery_mv = zmk_battery_state_of_charge_mv();
    
    LOG_INF("=== BATTERY STATUS ===");
    LOG_INF("Battery level: %d%%", battery_level);
    LOG_INF("Battery voltage: %d mV (%.2f V)", battery_mv, (float)battery_mv / 1000.0f);
    
    // Apagar todos los LEDs primero
    gpio_pin_set_dt(&led_green, 0);
    gpio_pin_set_dt(&led_red, 0);
    gpio_pin_set_dt(&led_yellow, 0);
    gpio_pin_set_dt(&led_blue, 0);
    
    if (battery_level > 80) {
        // Verde: batería muy alta (>80%)
        gpio_pin_set_dt(&led_green, 1);
        LOG_INF("LED Status: GREEN (Very High)");
    } else if (battery_level > 60) {
        // Azul: batería alta (60-80%)
        gpio_pin_set_dt(&led_blue, 1);
        LOG_INF("LED Status: BLUE (High)");
    } else if (battery_level > 30) {
        // Amarillo: batería media (30-60%)
        gpio_pin_set_dt(&led_yellow, 1);
        LOG_INF("LED Status: YELLOW (Medium)");
    } else {
        // Rojo: batería baja (<30%)
        gpio_pin_set_dt(&led_red, 1);
        LOG_INF("LED Status: RED (Low)");
    }
    LOG_INF("=====================");
}

// Listener para eventos de cambio de batería
static int battery_state_listener(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    
    update_battery_leds(ev->state_of_charge);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(battery_led_listener, battery_state_listener);
ZMK_SUBSCRIPTION(battery_led_listener, zmk_battery_state_changed);

// Inicialización del módulo
static int battery_led_init(void) {
    int ret;
    
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
    
    // Configurar LEDs como outputs
    ret = gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure green LED");
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&led_red, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure red LED");
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&led_yellow, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure yellow LED");
        return ret;
    }
    
    ret = gpio_pin_configure_dt(&led_blue, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Cannot configure blue LED");
        return ret;
    }
    
    // Obtener el nivel inicial de batería y mostrar información de inicio
    uint8_t initial_level = zmk_battery_state_of_charge();
    uint16_t initial_mv = zmk_battery_state_of_charge_mv();
    
    LOG_INF("=== BATTERY LED MODULE STARTED ===");
    LOG_INF("Initial battery level: %d%%", initial_level);
    LOG_INF("Initial battery voltage: %d mV", initial_mv);
    LOG_INF("LEDs configured on pins: G=10, R=16, Y=14, B=15");
    LOG_INF("================================");
    
    update_battery_leds(initial_level);
    
    LOG_INF("Battery LED module initialized");
    return 0;
}

SYS_INIT(battery_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);