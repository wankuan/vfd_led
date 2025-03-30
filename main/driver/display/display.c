#include <stdio.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "SPI_74HC595"

// 硬件连接
#define SPI_CLK_PIN    6  // SCK (时钟)
#define SPI_MOSI_PIN   7  // MOSI (数据)
#define SPI_CS_PIN     10  // CS (锁存)

// SPI 相关参数
#define SPI_HOST SPI2_HOST
#define NUM_74HC595 8  // 级联 8 个 74HC595

// 共阳极数码管 0-9 的字节映射
const uint8_t digit_map[10] = {
    0xB7, // 0
    0xA0, // 1
    0x3B, // 2
    0xB9, // 3
    0xAC, // 4
    0x9D, // 5
    0x9F, // 6
    0xB0, // 7
    0xBF, // 8
    0xBD  // 9
};

const uint8_t magic_num_separate = 0x08;
const uint8_t magic_num_dot_mask = 0x40;


// SPI 设备句柄
spi_device_handle_t spi;

#define GPIO_OUTPUT_PIN  0  // 定义 GPIO 0

void set_high_voltage_ouput() {
    // 配置 GPIO 0 为输出模式
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_OUTPUT_PIN),  // 选择 GPIO 0
        .mode = GPIO_MODE_OUTPUT,                   // 设置为输出模式
        .pull_up_en = GPIO_PULLUP_DISABLE,          // 不启用上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      // 不启用下拉
        .intr_type = GPIO_INTR_DISABLE              // 关闭中断
    };
    gpio_config(&io_conf); // 应用配置

    // 设置 GPIO 0 为高电平
    gpio_set_level(GPIO_OUTPUT_PIN, 1);

#define BEE_IO 4
    // 配置 GPIO 0 为输出模式
    gpio_config_t io_conf2 = {
        .pin_bit_mask = (1ULL << BEE_IO),  // 选择 GPIO 0
        .mode = GPIO_MODE_OUTPUT,                   // 设置为输出模式
        .pull_up_en = GPIO_PULLUP_DISABLE,          // 不启用上拉
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      // 不启用下拉
        .intr_type = GPIO_INTR_DISABLE              // 关闭中断
    };
    gpio_config(&io_conf2); // 应用配置
    gpio_set_level(BEE_IO, 0);

}

void display_init()
{
    set_high_voltage_ouput();
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,         // 无 MISO
        .mosi_io_num = SPI_MOSI_PIN,
        .sclk_io_num = SPI_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 16
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1MHz
        .mode = 0,   // SPI 模式 0
        .spics_io_num = SPI_CS_PIN, // CS 引脚
        .queue_size = 1,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST, &devcfg, &spi));
}

// 发送 8 个字节到 74HC595
void shift_out(uint8_t *data, int len) {
    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
    };
    ESP_ERROR_CHECK(spi_device_transmit(spi, &trans));
}

void send_8_group_data(uint8_t *data)
{
    uint8_t reverse_data[8];
    for(uint8_t index = 0; index < 8; ++index)
    {
        reverse_data[index] = data[7-index];
    }
    shift_out(reverse_data, 8);
}

uint8_t led_bit_mask[] =
{
    0x01,
    0x01 << 1,
    0x01 << 2,
    0x01 << 3,
    0x01 << 4,
    0x01 << 5,
    0x01 << 6,
    0x01 << 7
};

// 更新数码管显示时间
void display_time(int hour, int minute, int second)
{
    uint8_t buffer[NUM_74HC595] = {0};
    // // 以 HH-MM-SS 形式显示
    buffer[0] = digit_map[hour / 10];
    buffer[1] = digit_map[hour % 10];

    buffer[2] = magic_num_separate;

    buffer[3] = digit_map[minute / 10];
    buffer[4] = digit_map[minute % 10];

    buffer[5] = magic_num_separate;

    buffer[6] = digit_map[second / 10];
    buffer[7] = digit_map[second % 10];
    send_8_group_data(buffer);
}

// 更新数码管显示日期
void display_date(uint32_t year, uint32_t mon, uint32_t day)
{
    uint8_t buffer[NUM_74HC595] = {0};
    // // 以 YYYY.MM.DD 形式显示
    buffer[0] = digit_map[year / 1000];
    buffer[1] = digit_map[year % 1000 / 100];
    buffer[2] = digit_map[year % 100 / 10];
    buffer[3] = digit_map[year % 10] | magic_num_dot_mask;

    buffer[4] = digit_map[mon / 10];
    buffer[5] = digit_map[mon % 10] | magic_num_dot_mask;

    buffer[6] = digit_map[day / 10];
    buffer[7] = digit_map[day % 10] | magic_num_dot_mask;
    send_8_group_data(buffer);
}



// 定时器回调，每秒刷新时间
// void timer_callback(void *arg) {
//     static int sec = 0, min = 0, hour = 12;

//     sec++;
//     if (sec >= 60) {
//         sec = 0;
//         min++;
//     }
//     if (min >= 60) {
//         min = 0;
//         hour = (hour + 1) % 24;
//     }

//     display_time(hour, min, sec);
// }

// // 启动定时器，每秒刷新显示
// void start_timer() {
//     const esp_timer_create_args_t timer_args = {
//         .callback = &timer_callback,
//         .name = "time_display"
//     };
//     esp_timer_handle_t timer;
//     ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
//     ESP_ERROR_CHECK(esp_timer_start_periodic(timer, 1000000));  // 1s
// }

// 主函数
// void app_main() {
//     display_init();
//     start_timer();
// }


