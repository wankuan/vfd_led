#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO           8    // SCL引脚
#define I2C_MASTER_SDA_IO           9    // SDA引脚
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000    // I2C频率
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

#define DS3231_ADDR                 0x68   // DS3231 I2C 地址

#define TASK_PERIOD_MS    200   // 5Hz对应的周期是200毫秒

#define SEC_REG     (0x00)
#define MIN_REG     (0x01)
#define HOUR_REG    (0x02)

#define DAY_REG     (0x03)

#define DATE_REG    (0x04)
#define MON_REG     (0x05)
#define YEAR_REG    (0x06)

// 时间结构体
typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} time_t;

// I2C初始化
esp_err_t i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "I2C config failed");
        return ret;
    }

    ret = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    return ret;
}

esp_err_t ds3231_read_time(time_t *time) {
    uint8_t data[7];
    esp_err_t ret;
    uint8_t ds3231_time_reg = 0x00;

    ret = i2c_master_write_read_device(I2C_MASTER_NUM, DS3231_ADDR, &ds3231_time_reg, sizeof(ds3231_time_reg), data, 7, pdMS_TO_TICKS(200));
    if (ret != ESP_OK) {
        ESP_LOGE("DS3231", "Read time failed");
        return ret;
    }

    // 从返回的字节数据中解析时间
    time->sec = (data[0] & 0x7F);  // 秒
    time->min = data[1];           // 分
    time->hour = data[2];          // 小时
    time->day = data[3];           // 星期几
    time->date = data[4];          // 日
    time->month = data[5];         // 月
    time->year = data[6];          // 年

    ESP_LOGI("DS3231", "Current Time: %02x-%02x-%02x %02x:%02x:%02x week:%x",

        time->year,
        time->month,
        time->date,
        time->hour,
        time->min,
        time->sec,
        time->day
    );


    ESP_LOGI("DS3231", "Current Time: %04d-%02d-%02d %02d:%02d:%02d week:%d",
             2000 + time->year/16*10 +  time->year%16,
             time->month/16*10 +  time->month%16,
             time->date/16*10 +  time->date%16,
             time->hour/16*10 +  time->hour%16,
             time->min/16*10 +  time->min%16,
             time->sec/16*10 +  time->sec%16,
             time->day%16);

    return ESP_OK;
}


void ds3231_set_date(uint32_t year, uint32_t mon, uint32_t date, uint32_t day)
{
    uint8_t data[2];

    // 年仅支持十分位和个位。例2025，则仅支持设置25
    year = year%100;

    data[0] = YEAR_REG;
    data[1] = year/10*16 + year%10;
    i2c_master_write_to_device(I2C_MASTER_NUM, DS3231_ADDR, data, sizeof(data), pdMS_TO_TICKS(200));

    data[0] = MON_REG;
    data[1] = mon/10*16 + mon%10;
    i2c_master_write_to_device(I2C_MASTER_NUM, DS3231_ADDR, data, sizeof(data), pdMS_TO_TICKS(200));

    data[0] = DATE_REG;
    data[1] = date/10*16 + date%10;
    i2c_master_write_to_device(I2C_MASTER_NUM, DS3231_ADDR, data, sizeof(data), pdMS_TO_TICKS(200));

    data[0] = DAY_REG;
    data[1] = day/10*16 + day%10;
    i2c_master_write_to_device(I2C_MASTER_NUM, DS3231_ADDR, data, sizeof(data), pdMS_TO_TICKS(200));
}


// esp_err_t ds3231_set_time(time_t *time)
// {
//     uint8_t data[8];
//     data[0] = READ_TIME_REG;
//     // 将输入的十进制时间转换为BCD格式
//     data[1] = (time->sec / 10) << 4 | (time->sec % 10);  // 秒
//     data[2] = (time->min / 10) << 4 | (time->min % 10);  // 分
//     data[3] = (time->hour / 10) << 4 | (time->hour % 10); // 小时
//     data[4] = (time->day / 10) << 4 | (time->day % 10);   // 星期几
//     data[5] = (time->date / 10) << 4 | (time->date % 10);  // 日
//     data[6] = (time->month / 10) << 4 | (time->month % 10); // 月
//     data[7] = (time->year / 10) << 4 | (time->year % 10);  // 年

//     // 通过I2C向DS3231写入数据
//     return i2c_master_write_to_device(I2C_MASTER_NUM, DS3231_ADDR, data, sizeof(data), pdMS_TO_TICKS(200));
// }

void display_time(int hour, int minute, int second);
void display_date(uint32_t year, uint32_t mon, uint32_t day);

static void clock_task(void *arg)
{
    esp_err_t ret = ESP_OK;
    TickType_t xLastWakeTime = xTaskGetTickCount();  // 获取任务开始的时间戳
    while(1)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TASK_PERIOD_MS));
        time_t current_time;
        // 读取当前时间
        ret = ds3231_read_time(&current_time);
        uint8_t hour = 0;
        uint8_t min = 0;
        uint8_t sec = 0;
        hour = current_time.hour/16*10 + current_time.hour%16;
        min = current_time.min/16*10 + current_time.min%16;
        sec = current_time.sec/16*10 + current_time.sec%16;

        ESP_LOGI("Main", "time: %02d - %02d - %02d", hour, min, sec);
        display_time(hour, min, sec);

        hour = current_time.hour/16*10 + current_time.hour%16;
        min = current_time.min/16*10 + current_time.min%16;
        sec = current_time.sec/16*10 + current_time.sec%16;
        if(sec == 0 || sec == 1 || sec == 30 || sec == 31)
        {
            display_date(
                2000 + current_time.year/16*10 +  current_time.year%16,
                current_time.month/16*10 +  current_time.month%16,
                current_time.date/16*10 +  current_time.date%16);
        }

        if (ret != ESP_OK)
        {
            ESP_LOGI("Main", "ds3231 read fail! ret:%d", ret);
        }
    }

}

void clock_init()
{
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE("I2C", "I2C init failed");
        return;
    }
    ds3231_set_date(2025, 3, 30, 7);
    xTaskCreate(clock_task, "clock_task", 2048, NULL, 5, NULL);

    // // 设置新的时间
    // time_t new_time = {0x45, 0x30, 0x10, 0x06, 0x28, 0x03, 0x2025};  // 设置2025年3月28日10:30:45
    // ret = ds3231_set_time(&new_time);
    // if (ret == ESP_OK) {
    //     ESP_LOGI("Main", "New time set");
    // }
}