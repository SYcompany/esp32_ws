#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "driver/pcnt.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <inttypes.h>

#define GPIO_PWM0A_OUT 4    //Set GPIO 15 as PWM0A
#define GPIO_INPUT_IO_0  16
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0))

SemaphoreHandle_t g_motor_mux;
float a = 0;
int control_case = 0;

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


static void gpio_task_example(void* arg)
{
    uint32_t io_num;
   
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", io_num, gpio_get_level(io_num));
            control_case++;
        }
    }
}

static void mcpwm_example_gpio_initialize(void)
{
    printf("initializing mcpwm gpio...\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
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
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

static void mcpwm_brushed_motor_receive_thread(void *arg)
{
    while(1)
    {
        xSemaphoreTake(g_motor_mux,portTICK_PERIOD_MS);
        a = 80.f;
        if(control_case == 0)
        {
            gpio_set_level(15,1);
            //control_case = 1;
        }
        else if(control_case == 1)
        {
            gpio_set_level(15,0);
            //control_case = 2;
        }
        else if(control_case == 2)
        {
            gpio_set_level(2,1);
            //control_case = 3;
        }
        else if(control_case == 3)
        {
            
            gpio_set_level(2,0);
            //control_case = 0;
        }
        else{control_case = 0;}
        
        xSemaphoreGive(g_motor_mux);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    gpio_config_t io_conf = {};
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO16 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio interrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_POSEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
    
    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);


    mcpwm_example_gpio_initialize();
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;    // PWM 주파수: 1000 Hz
    pwm_config.cmpr_a = 0;          // 초기 듀티 사이클: 0%
    pwm_config.cmpr_b = 0;          // 초기 듀티 사이클: 0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    g_motor_mux = xSemaphoreCreateMutex();

    gpio_reset_pin(15);
    gpio_reset_pin(2);
    gpio_set_direction(15, GPIO_MODE_OUTPUT);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    while(1)
    {
        //xTaskCreate(mcpwm_brushed_motor_ctrl_thread, "mcpwm_brushed_motor_ctrl_thread", 4096, NULL, 1, NULL);
        //xTaskCreate(mcpwm_brushed_motor_receive_thread, "mcpwm_brushed_motor_recieve_thread", 4096, NULL, 0, NULL);
        xTaskCreatePinnedToCore(mcpwm_brushed_motor_ctrl_thread, "mcpwm_brushed_motor_ctrl_thread", 4096, NULL, 1, NULL,0);
        xTaskCreatePinnedToCore(mcpwm_brushed_motor_receive_thread, "mcpwm_brushed_motor_recieve_thread", 4096, NULL, 1, NULL,1);
    }
    
    
}
