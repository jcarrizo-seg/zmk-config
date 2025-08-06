# CMakeLists.txt para Battery Status LEDs Module
# Archivo: config/boards/shields/mi_teclado/CMakeLists.txt

# Agregar el archivo source al build si ZMK está habilitado
if(CONFIG_ZMK)
    # Agregar el archivo battery_status_leds.c al build
    target_sources(app PRIVATE battery_status_leds.c)
    
    # Log para confirmar que se incluye en el build
    message(STATUS "Battery Status LEDs module included in build")
endif()