#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "driver/pcnt.h"


#define GPIO_PWM0A_OUT 2    //Set GPIO 15 as PWM0A
#define GPIO_PWM0B_OUT 3   //Set GPIO 16 as PWM0B

SemaphoreHandle_t g_motor_mux;
float a = 0;

static void mcpwm_example_gpio_initialize(void)
{
    printf("initializing mcpwm gpio...\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT);
}


void brushed_motor_set_duty(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float speed)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, speed);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}



static void mcpwm_brushed_motor_ctrl_thread(void *arg)
{
    while(1)
    {
        xSemaphoreTake(g_motor_mux,portTICK_PERIOD_MS);
        brushed_motor_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0,a);
        xSemaphoreGive(g_motor_mux);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void mcpwm_brushed_motor_receive_thread(void *arg)
{
    while(1)
    {
        xSemaphoreTake(g_motor_mux,portTICK_PERIOD_MS);
        a += 0.01f;
        if(a > 100)
        {
            a = 0;
        }
        
        xSemaphoreGive(g_motor_mux);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    mcpwm_example_gpio_initialize();
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;    // PWM 주파수: 1000 Hz
    pwm_config.cmpr_a = 0;          // 초기 듀티 사이클: 0%
    pwm_config.cmpr_b = 0;          // 초기 듀티 사이클: 0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    g_motor_mux = xSemaphoreCreateMutex();
    while(1)
    {
        xTaskCreate(mcpwm_brushed_motor_ctrl_thread, "mcpwm_brushed_motor_ctrl_thread", 4096, NULL, 1, NULL);
        xTaskCreate(mcpwm_brushed_motor_receive_thread, "mcpwm_brushed_motor_recieve_thread", 4096, NULL, 0, NULL);
    }
    
    
}
