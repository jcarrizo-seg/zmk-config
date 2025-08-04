# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_USB_LED)
  target_sources(app PRIVATE src/usb_led.c)
endif()