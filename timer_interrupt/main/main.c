#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
#include "esp_intr_alloc.h"
#include "esp_attr.h"

#define TIMER_DIVIDER         16  // 하드웨어 타이머 클럭 분주기
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // 타이머가 초 단위로 계산
#define TIMER_INTERVAL_SEC   (3.0) // 타이머 인터럽트 간격 (초 단위)

// 타이머 관련 정보를 저장할 구조체
typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} timer_info_t;

// 인터럽트 핸들러
void IRAM_ATTR timer_group0_isr(void *param)
{
    timer_info_t *info = (timer_info_t *) param;
    // 타이머 인터럽트 플래그 해제
    TIMERG0.int_clr_timers.t0 = 1;
    // 자동 리로드가 아닐 경우, 수동으로 리로드
    if (!info->auto_reload) {
        timer_set_counter_value(info->timer_group, info->timer_idx, 0x00000000ULL);
        timer_set_alarm_value(info->timer_group, info->timer_idx, info->alarm_interval * TIMER_SCALE);
        timer_start(info->timer_group, info->timer_idx);
    }
    // 여기서 ISR을 사용한 작업을 수행
    printf("Timer Interrupt Triggered\n");
}

void app_main(void)
{
    timer_info_t timer_info = {
        .timer_group = TIMER_GROUP_0,
        .timer_idx = TIMER_0,
        .alarm_interval = TIMER_INTERVAL_SEC,
        .auto_reload = true
    };

    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = timer_info.auto_reload,
    };

    // 타이머 설정
    timer_init(timer_info.timer_group, timer_info.timer_idx, &config);
    // 카운터 값 설정
    timer_set_counter_value(timer_info.timer_group, timer_info.timer_idx, 0x00000000ULL);
    // 알람 값 설정
    timer_set_alarm_value(timer_info.timer_group, timer_info.timer_idx, timer_info.alarm_interval * TIMER_SCALE);
    // 인터럽트 활성화 및 설정
    timer_enable_intr(timer_info.timer_group, timer_info.timer_idx);
    timer_isr_register(timer_info.timer_group, timer_info.timer_idx, timer_group0_isr, (void *) &timer_info, ESP_INTR_FLAG_IRAM, NULL);

    // 타이머 시작
    timer_start(timer_info.timer_group, timer_info.timer_idx);
}

