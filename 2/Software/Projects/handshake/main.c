#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "bsp.h"

TaskHandle_t task_a_t;
TaskHandle_t task_b_t;



void task_a(void *args);
void task_b(void *args);

/*************************************************************/

/**
 * @brief Main function.
 *
 * @return int
 */
int main()
{
    BSP_Init();             /* Initialize all components on the lab-kit. */

    /* Create the tasks. */
    xTaskCreate(task_a, "Task A", 512, NULL, 2, &task_a_t);

    xTaskCreate(task_b, "Task B", 512, NULL, 2, &task_b_t);


    vTaskStartScheduler();  /* Start the scheduler. */

    while (true) {
        sleep_ms(1000); /* Should not reach here... */
    }
}

void task_a(void *args) {
    TickType_t xLastWakeTime = 0;

    for (;;) {
        BSP_SetLED(LED_GREEN, 1);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
        BSP_SetLED(LED_GREEN, 0);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(4000));
        BSP_SetLED(LED_GREEN, 1);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(4000));


        // syncing is probably not needed unless you run it for 16 hours
        xTaskNotify(task_b_t, 0, eNoAction);
    }
}


void task_b(void *args) {
    TickType_t xLastWakeTime = 0;

    for (;;) {
        BSP_SetLED(LED_RED, 1);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(4000));
        BSP_SetLED(LED_RED, 0);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(4000));
        BSP_SetLED(LED_RED, 1);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
        uint32_t val;
        xTaskNotifyWait(0, 0, &val, portMAX_DELAY);
    }
}