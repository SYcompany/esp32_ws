#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include <driver/adc.h>

#define LED_GPIO_PIN 2 // ESP32 보드에 따라 적절한 GPIO 핀 번호로 변경하세요.


// LED를 깜박이는 태스크
void blink_task(void *pvParameter) {
    gpio_pad_select_gpio(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(LED_GPIO_PIN, 0); // LED 켜기
        vTaskDelay(10 / portTICK_PERIOD_MS); // 1초 대기
        gpio_set_level(LED_GPIO_PIN, 1); // LED 끄기
        vTaskDelay(10 / portTICK_PERIOD_MS); // 1초 대기
    }
}

// 시간을 출력하는 태스크
void print_time_task(void *pvParameter) {
    while (1) {
        printf("Tick: %f\n", esp_timer_get_time() / 1000000.f);
        printf("Voltage: %d\n", adc1_get_raw(ADC1_CHANNEL_4));
        vTaskDelay(10 / portTICK_PERIOD_MS); // 1초 대기
    }
}

void app_main() {
    //static esp_adc_cal_characteristics_t adc1_chars;
    //esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chars);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_4,ADC_ATTEN_DB_11);
    
    xTaskCreatePinnedToCore(blink_task, "blink_task", 2048, NULL, 5, NULL, 0); // 코어 0에 할당
    xTaskCreatePinnedToCore(print_time_task, "print_time_task", 2048, NULL, 5, NULL, 1); // 코어 1에 할당
}

