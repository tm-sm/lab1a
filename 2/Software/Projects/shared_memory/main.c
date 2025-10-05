#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "bsp.h"

volatile uint32_t shared_address;

TaskHandle_t task_a_t;
TaskHandle_t task_b_t;

void task_a(void *args);
void task_b(void *args);

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
    uint32_t i = 0;
    while (true) {
        i++;
        printf("Sending %d\n", i);
        shared_address = i;
        xTaskNotify(task_b_t, 0, eNoAction);
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        printf("Receiving %d\n", shared_address);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_b(void *args) {
    for (;;) {
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        shared_address *= -1;
        xTaskNotify(task_a_t, 0, eNoAction);

    }
}
