set(srcs
"led_strip_rmt_ws2812.c"
"demo.c"
)
idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES "driver"
                    )
