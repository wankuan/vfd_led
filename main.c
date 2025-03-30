#include <stdio.h>
#include "freertos/FreeRTOS.h"


void display_time(int hour, int minute, int second);
void rgb_main_init();

void app_main(void)
{
    rgb_main_init();
}
