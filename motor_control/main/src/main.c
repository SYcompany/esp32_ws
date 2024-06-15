#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO       (2) // GPIO 번호는 필요에 따라 변경하세요
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_TEST_DUTY         (4000)
#define LEDC_TEST_FADE_TIME    (3000)

void app_main() {
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // 해상도 13-bit
        .freq_hz = 5000,                      // PWM 주파수: 5000 Hz
        .speed_mode = LEDC_HS_MODE,           // 타이머의 속도 모드
        .timer_num = LEDC_HS_TIMER            // 사용할 타이머
    };
    // 타이머 설정
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_HS_CH0_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_HS_CH0_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER
    };
    // 채널 설정
    ledc_channel_config(&ledc_channel);

    // 페이드 기능을 사용하여 PWM 신호를 변조
    ledc_fade_func_install(ESP_INTR_FLAG_LEVEL1);

    while (1) {
        // 듀티 사이클 증가
        ledc_set_fade_with_time(ledc_channel.speed_mode,
                                ledc_channel.channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME);
        ledc_fade_start(ledc_channel.speed_mode,
                        ledc_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

        // 듀티 사이클 감소
        ledc_set_fade_with_time(ledc_channel.speed_mode,
                                ledc_channel.channel, 0, LEDC_TEST_FADE_TIME);
        ledc_fade_start(ledc_channel.speed_mode,
                        ledc_channel.channel, LEDC_FADE_NO_WAIT);
        vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
    }
}
