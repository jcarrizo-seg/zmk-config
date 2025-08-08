# CMakeLists.txt para incluir código personalizado

if(CONFIG_SHIELD_SINGLE)
    target_sources(app PRIVATE battery_led.c)
endif()