# CMakeLists.txt para Battery Status LEDs Module
# Archivo: config/boards/shields/mi_teclado/CMakeLists.txt

# Verificar que el nodo battery_status_leds esté habilitado antes de compilar
if(CONFIG_ZMK AND DT_HAS_COMPAT_STATUS_OKAY(zmk_battery_status_leds))
    # Agregar el archivo source al build
    target_sources(app PRIVATE battery_status_leds.c)
    
    # Log para confirmar que se incluye en el build
    message(STATUS "Battery Status LEDs module enabled")
else()
    # Log si no está habilitado (para debugging)
    message(STATUS "Battery Status LEDs module not enabled (missing devicetree node)")
endif()