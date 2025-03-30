set(srcs
"driver/rgb/led_strip_rmt_ws2812.c"
"driver/rgb/rgb.c"
"driver/clock/clock.c"
"driver/display/display.c"
"main.c"
)
idf_component_register(SRCS ${srcs}
                    # INCLUDE_DIRS "include"
                    PRIV_REQUIRES "driver"
                    )
