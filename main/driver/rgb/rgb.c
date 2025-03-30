#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "led_strip.h"
#include "driver/rmt.h"
#include "sdkconfig.h"

static led_strip_t *strip;

#define LIGHTNESS_MAX 255
#define LIGHT_INTERVAL 2

static void rgb_task(void *arg)
{
    static uint8_t reverse_flag = 0;
    static int16_t lightness = 0;
    static uint8_t state_cnt = 1;

    static uint8_t r = 0;
    static uint8_t g = 0;
    static uint8_t b = 0;

    while(1)
    {
        if(reverse_flag == 0)
        {
            lightness += LIGHT_INTERVAL;
            if(lightness > LIGHTNESS_MAX)
            {
                lightness = LIGHTNESS_MAX;
                reverse_flag = 1;
            }
        }
        else
        {
            lightness -= LIGHT_INTERVAL;
            if(lightness < 0)
            {
                lightness = 0;
                reverse_flag = 0;
                state_cnt++;
                if(state_cnt > 7)
                {
                    state_cnt = 1;
                }
            }
        }

        r = 0;
        g = 0;
        b = 0;
        if(state_cnt & 0x04)
        {
            r = lightness;
        }
        if(state_cnt & 0x02)
        {
            g = lightness;
        }
        if(state_cnt & 0x01)
        {
            b = lightness;
        }

        for(uint8_t i = 0; i < CONFIG_RGB_NUMS; i++)
        {
            strip->set_pixel(strip, i, r, g, b);
        }
        strip->refresh(strip, 100);
        // printf("set rgb state_cnt:%d, lightness:%d...\n", state_cnt, lightness);
        vTaskDelay(40 / portTICK_PERIOD_MS);
    }
}



void rgb_main_init(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_RGB_IO, RMT_CHANNEL_0);
    config.clk_div = 2; // set counter clock to 40MHz
    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);

    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_RGB_NUMS, (led_strip_dev_t) config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    strip->clear(strip, 100); 
    
    xTaskCreate(rgb_task, "rgb_task", 2048, NULL, 5, NULL);
}
