# CMakeLists.txt para Battery Status LEDs Module
# Archivo: config/boards/shields/mi_teclado/CMakeLists.txt

target_sources(app PRIVATE battery_status_leds.c)
    
# Log para confirmar que se incluye en el build
message(STATUS "Battery Status LEDs module included in build")