#include <stdio.h>
#include "freertos/FreeRTOS.h"


void rgb_main_init();
void clock_init();

void display_init();

void app_main(void)
{
    rgb_main_init();
    clock_init();
    display_init();

}
