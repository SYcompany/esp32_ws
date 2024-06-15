#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "esp_intr_alloc.h"
#include "driver/timer.h"
#include "freertos/portmacro.h"

#define GPIO_PWM0A_OUT 2   // Motor driver input pin 1
#define GPIO_PWM0B_OUT 33  // Motor driver input pin 2
#define TIMER_DIVIDER         16
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)
#define TIMER_INTERVAL_SEC   (3.0)

volatile float spd = 0;
volatile bool sw = false;

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
    portMUX_TYPE timer_mux;  // Critical section management
} timer_info_t;

void IRAM_ATTR timer_group0_isr(void *param)
{
    timer_info_t *info = (timer_info_t *) param;
    TIMERG0.int_clr_timers.t0 = 1;
    portENTER_CRITICAL_ISR(&info->timer_mux);
    
    
    if(sw == true)
    {
        spd = 100.f;
        sw = false;
    }
    else
    {
        spd = 0.f;
        sw = true;
        
    }

    portEXIT_CRITICAL_ISR(&info->timer_mux);
}

void mcpwm_example_gpio_initialize(void)
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT);
}

void brushed_motor_forward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, float speed)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, speed);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

void app_main(void)
{
    printf("start?");
    mcpwm_example_gpio_initialize();

    mcpwm_config_t pwm_config = {
        .frequency = 1000,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    timer_info_t timer_info = {
        .timer_group = TIMER_GROUP_0,
        .timer_idx = TIMER_0,
        .alarm_interval = TIMER_INTERVAL_SEC,
        .auto_reload = true,
        .timer_mux = portMUX_INITIALIZER_UNLOCKED  // Initialize the spinlock
    };
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = timer_info.auto_reload,
    };
    
    timer_init(timer_info.timer_group, timer_info.timer_idx, &config);
    timer_set_counter_value(timer_info.timer_group, timer_info.timer_idx, 0x00000000ULL);
    timer_set_alarm_value(timer_info.timer_group, timer_info.timer_idx, timer_info.alarm_interval * TIMER_SCALE);
    timer_enable_intr(timer_info.timer_group, timer_info.timer_idx);
    timer_isr_register(timer_info.timer_group, timer_info.timer_idx, timer_group0_isr, (void *) &timer_info, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(timer_info.timer_group, timer_info.timer_idx);

    while (1)
    {
        //vTaskDelay(pdMS_TO_TICKS(100));
        brushed_motor_forward(MCPWM_UNIT_0, MCPWM_TIMER_0, spd);
    }
}
