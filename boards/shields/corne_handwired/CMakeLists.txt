if(CONFIG_ZMK_DISPLAY AND CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM)
  zephyr_library_sources(custom_status_screen.c)
  zephyr_library_sources(widgets/util.c)

  # Layer events only link on the central image (zmk/app/CMakeLists.txt:48
  # gates keymap.c + layer_state_changed.c on CONFIG_ZMK_SPLIT_ROLE_CENTRAL).
  # Right half has no display anyway, so just compile the widget on central.
  if(NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zephyr_library_sources(widgets/status.c)
  endif()
endif()
