if(CONFIG_SHIELD_CORNE_HANDWIRED_LEFT)
    zephyr_library()
    zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    # ZMK's app/include is private to the `app` library — pull it in
    # explicitly so our custom widgets can use <zmk/display.h>, etc.
    zephyr_library_include_directories(${ZEPHYR_BASE}/../zmk/app/include)
    zephyr_library_sources(custom_status_screen.c)
    zephyr_library_sources(widgets/layer_status.c)
endif()
