/**
 * @file main.c
 * @author Ingo Sander (ingo@kth.se)
 * @brief Skeleton for cruise control application
 *        The skeleton code runs on the ES-Lab-Kit,
 *        has very limited functionality and needs to be
 *        modified.
 *
 * @version 0.1
 * @date 2025-09-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "bsp.h"
#include "hardware/clocks.h"

// #define USE_ACCEL

#define GAS_STEP 2  /* Defines how much the throttle is increased if GAS_STEP is asserted */

/* Definition of handles for tasks */
TaskHandle_t    xButton_handle; /* Handle for the Button task */
TaskHandle_t    xControl_handle; /* Handle for the Control task */
TaskHandle_t    xVehicle_handle; /* Handle for the Vehicle task */
TaskHandle_t    xDisplay_handle; /* Handle for the Display task */
TaskHandle_t    xOverload_handle; /* Handle for the Display task */
TaskHandle_t    xWatchdog_handle; /* Handle for the Display task */
TaskHandle_t    xLoad_handle; /* Handle for the Display task */

/* Definition of handles for queues */
QueueHandle_t xQueueVelocity;
QueueHandle_t xQueuePosition;
QueueHandle_t xQueueThrottle;
QueueHandle_t xQueueCruiseControl;
QueueHandle_t xQueueGasPedal;
QueueHandle_t xQueueBrakePedal;
QueueHandle_t xQueueSlope;


void vLoadTask(void *args) {
    TickType_t period = (TickType_t) args;
    TickType_t xLastWakeTime = 0;

    while (true) {
        uint8_t load = 0;
        load |= BSP_GetInput(SW_17) << 0;
        load |= BSP_GetInput(SW_16) << 1;
        load |= BSP_GetInput(SW_15) << 2;
        load |= BSP_GetInput(SW_14) << 3;
        load |= BSP_GetInput(SW_13) << 4;
        load |= BSP_GetInput(SW_12) << 5;
        load |= BSP_GetInput(SW_11) << 6;
        load |= BSP_GetInput(SW_10) << 7;
        uint64_t start = to_ms_since_boot(get_absolute_time());
        while (to_ms_since_boot(get_absolute_time()) - start < load / 10) {
        }
        vTaskDelayUntil(&xLastWakeTime, period);
    }
}

void vWatchdogTask(void *args) {
    TickType_t last_kick = xTaskGetTickCount();
    bool overloaded = false;
    TickType_t period = (TickType_t) args;
    while (true) {
        int ret = xTaskNotifyWait(0, 0, 0, pdMS_TO_TICKS(period));
        if (ret == pdFALSE) {
            overloaded = true;
            BSP_SetLED(LED_GREEN, 1);
            BSP_SetLED(LED_RED, 1);
            BSP_SetLED(LED_YELLOW, 1);
            printf("overloaded\n");
        }
        else if (overloaded) {
            overloaded = false;
            BSP_SetLED(LED_GREEN, 0);
            BSP_SetLED(LED_RED, 0);
            BSP_SetLED(LED_YELLOW, 0);
        }
    }
}

void vOverloadTask(void *args) {
    TickType_t xLastWakeTime = 0;
    TickType_t xLastLastWakeTime = 0;
    TickType_t period = (TickType_t) args;
    while (true) {
        xLastLastWakeTime = xLastWakeTime;
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(period));
        if (xLastWakeTime - xLastLastWakeTime < (period * 2)) {
            xTaskNotify(xWatchdog_handle, 0, eNoAction);
        }
    }
}

/**
 * @brief The button task shall monitor the input buttons and send the values to the
 *        other tasks
 *
 * ==> MODIFY THIS TASK!
 *     Currently the buttons are ignored. Use busy wait I/O to monitor the buttons
 *
 * @param args
 */
void vButtonTask(void *args) {
    bool value_gas_pedal = true;
    bool value_brake_pedal = false;
    bool value_cruise_control = false;
    uint32_t period = (uint32_t) args;
    TickType_t xLastWakeTime = 0;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(period));
        value_brake_pedal = !BSP_GetInput(SW_5); // active low
        value_cruise_control = !BSP_GetInput(SW_6);  // active low
        value_gas_pedal = !BSP_GetInput(SW_7); // active low

        xQueueOverwrite(xQueueGasPedal,&value_gas_pedal);
        xQueueOverwrite(xQueueBrakePedal,&value_brake_pedal);
        xQueueOverwrite(xQueueCruiseControl,&value_cruise_control);
    }
}

/**
 * @brief The control tasks calculates the new throttle using your
 *        control algorithm and the current values.
 *
 * ==> MODIFY THIS TASK!
 *     Currently the throttle has a fixed value of 80
 *
 * @param args
 */
void vControlTask(void *args) {
    uint16_t throttle = 80;
    uint16_t velocity;
    uint16_t cruise_velocity;
    bool cruise_control_button;
    bool cruise_control = false;
    bool gas_pedal;
    bool brake_pedal;

    uint32_t period = (uint32_t) args;
    TickType_t xLastWakeTime = 0;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(period));

        xQueuePeek(xQueueCruiseControl, &cruise_control_button, ( TickType_t ) 0);
        xQueuePeek(xQueueGasPedal, &gas_pedal, ( TickType_t ) 0);
        xQueuePeek(xQueueVelocity, &velocity, ( TickType_t ) 0);
        xQueuePeek(xQueueBrakePedal, &brake_pedal, ( TickType_t ) 0);

        if (cruise_control_button) {
            cruise_velocity = velocity;
            cruise_control = true;
        }

        if (velocity <= 250) {
            cruise_control = false;
        }

        if (gas_pedal) {
            throttle += GAS_STEP;
            if (throttle > 80) {
                throttle = 80;
            }
            cruise_control = false;
        }
        else if (brake_pedal) {
            throttle = 0;
            cruise_control = false;
        }
        else if (cruise_control) {
            throttle = 80 * (velocity < cruise_velocity);
        }

        BSP_SetLED(LED_YELLOW, cruise_control);
        xQueueOverwrite(xQueueThrottle, &throttle);
    }
}

/**
 * @brief The function returns the new position depending on the input parameters.
 *
 * ==> DO NOT CHANGE THIS FUNCTION !!!
 *
 * @param position
 * @param velocity
 * @param acceleration
 * @param time_interval
 * @return
 */
uint16_t adjust_position(uint16_t position, int16_t velocity,
                         int8_t acceleration, uint16_t time_interval)
{
  int16_t new_position = position + velocity * time_interval / 1000
    + acceleration / 2  * (time_interval / 1000) * (time_interval / 1000);

  if (new_position > 24000) {
    new_position -= 24000;
  } else if (new_position < 0){
    new_position += 24000;
  }

  return new_position;
}


/**
 * @brief The function returns the new velocity depending on the input parameters.
 *
 * ==> DO NOT CHANGE THIS FUNCTION !!!
 *
 * @param velocity
 * @param acceleration
 * @param brake_pedal
 * @param time_interval
 * @return
 */
int16_t adjust_velocity(int16_t velocity, int8_t acceleration,
		       bool brake_pedal, uint16_t time_interval)
{
  int16_t new_velocity;
  uint8_t brake_retardation = 50;

  if (brake_pedal == false) {
    new_velocity = velocity  + (float) ((acceleration * time_interval) / 1000);
    if (new_velocity <= 0) {
        new_velocity = 0;
    }
  }
  else {
    if ((float) (brake_retardation * time_interval) / 1000 > velocity) {
       new_velocity = 0;
    }
    else {
      new_velocity = velocity - (float) brake_retardation * time_interval / 1000;
    }
  }

  return new_velocity;
}

/**
 * @brief The vehicle task continuously calculates the velocity of the vehicle
 *
 * ==> DO NOT CHANGE THIS TASK !!!
 *
 * @param args
 */
void vVehicleTask(void *args) {
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */
    uint16_t throttle;
    bool brake_pedal;
                           /* Approximate values*/
    uint8_t acceleration;  /* Value between 40 and -20 (4.0 m/s^2 and -2.0 m/s^2) */
    uint8_t retardation;   /* Value between 20 and -10 (2.0 m/s^2 and -1.0 m/s^2) */
    uint16_t position = 0; /* Value between 0 and 24000 (0.0 m and 2400.0 m)  */
    uint16_t velocity = 0; /* Value between -200 and 700 (-20.0 m/s amd 70.0 m/s) */
    uint16_t wind_factor;   /* Value between -10 and 20 (2.0 m/s^2 and -1.0 m/s^2) */
    uint16_t slope = 0;
    float acc = 1.0;
    float alpha = 0.2;
    for (;;) {
        xQueuePeek(xQueueThrottle, &throttle, ( TickType_t ) 0);
        xQueuePeek(xQueueBrakePedal, &brake_pedal, ( TickType_t ) 0);

        /* Retardation : Factor of Terrain and Wind Resistance */
        if (velocity > 0)
	        wind_factor = velocity * velocity / 10000 + 1;
        else
	        wind_factor = (-1) * velocity * velocity / 10000 + 1;
        #ifdef USE_ACCEL
        acc += alpha * (BSP_GetAxisAcceleration(Z_AXIS) - acc);
        slope = (1.05 - acc) * 90.0;
        if (slope < 0) {
            slope = 0;
        }
        else if (slope > 90) {
            slope = 90;
        }
        printf("slope %d acc %f\n", slope, acc);
        retardation = wind_factor + slope;
        #else
        if (position < 4000)
            retardation = wind_factor; // even ground
        else if (position < 8000)
            retardation = wind_factor + 8; // traveling uphill
        else if (position < 12000)
            retardation = wind_factor + 16; // traveling steep uphill
        else if (position < 16000)
            retardation = wind_factor; // even ground
        else if (position < 20000)
            retardation = wind_factor - 8; //traveling downhill
        else
            retardation = wind_factor - 16 ; // traveling steep downhill
        #endif
        acceleration = throttle / 2 - retardation;
        position = adjust_position(position, velocity, acceleration, xPeriod);
        velocity = adjust_velocity(velocity, acceleration, brake_pedal, xPeriod);


        xQueueOverwrite(xQueueVelocity, &velocity);
        xQueueOverwrite(xQueuePosition, &position);
        xQueueOverwrite(xQueueSlope, &slope);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Wait for the next release. */
    }
}

/**
 * @brief The display task shall show the information on
 *          - the throttle and velocity on the seven segment display
 *          - the position on the 24 LEDs
 *
 * ==> MODIFY THIS TASK!
 *     Currently the information is shown on the standard output (serial monitor in VSCode)
 *
 * @param args
 */
void vDisplayTask(void *args) {
    TickType_t xLastWakeTime = 0;
    const TickType_t xPeriod = (int)args;   /* Get period (in ticks) from argument. */

    uint16_t velocity;
    uint16_t throttle;
    uint16_t position;
    uint16_t slope;
    BSP_7SegClear();
    BSP_7SegBrightness(15);

    for (;;) {
        xQueuePeek(xQueueVelocity, &velocity, ( TickType_t ) 0);
        xQueuePeek(xQueuePosition, &position, ( TickType_t ) 0);
        xQueuePeek(xQueueThrottle, &throttle, ( TickType_t ) 0);
        xQueuePeek(xQueueSlope, &slope, ( TickType_t ) 0);

        printf("Throttle: %d\n", throttle);
        printf("Velocity: %d\n", velocity);
        printf("Position: %d\n", position);

        #ifdef USE_ACCEL
        uint16_t first = slope;
        #else
        uint16_t first = throttle;
        #endif

        char buf[16];
        sprintf(buf, "%02d%02d", first % 100, (velocity / 10) % 100);
        BSP_7SegDispString(buf);
        uint8_t leds[3] = {0, 0, 0};
        uint8_t led_index = (position % 2400) / 100;

        leds[led_index / 8] |= 1 << (led_index % 8);;
        BSP_ShiftRegWriteAll(leds);

        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* Wait for the next release. */
    }
}

/**
 * @brief Main program that starts all the tasks and the scheduler
 *
 * ==> MODIFY THE MAIN PROGRAM!
 *        - Convert the button and control tasks to periodic tasks
 *        - Adjust the priorities of the task so that they correspond
 *          to the rate-monotonic algorithm.
 * @return
 */
int main()
{
    BSP_Init();  /* Initialize all components on the ES Lab-Kit. */

    /* Create the tasks. */
    xTaskCreate(vButtonTask, "Button Task", 512, (void*) 50, 6, &xButton_handle);
    xTaskCreate(vVehicleTask, "Vehicle Task", 512, (void*) 100, 5, &xVehicle_handle);
    xTaskCreate(vControlTask, "Control Task", 512, (void*) 200, 4, &xControl_handle);
    xTaskCreate(vDisplayTask, "Display Task", 512, (void*) 500, 3, &xDisplay_handle);

    xTaskCreate(vWatchdogTask, "Watchdog Task", 512, (void*) 1000, 7, &xWatchdog_handle);
    xTaskCreate(vOverloadTask, "Overload Task", 512, (void*) 25, 1, &xOverload_handle);
    xTaskCreate(vLoadTask, "Load Task", 512, (void*) 25, 2, &xLoad_handle);

    /* Create the message queues */
    xQueueCruiseControl = xQueueCreate( 1, sizeof(bool));
    xQueueGasPedal = xQueueCreate( 1, sizeof(bool));
    xQueueBrakePedal = xQueueCreate( 1, sizeof(bool));
    xQueueVelocity = xQueueCreate( 1, sizeof(uint16_t));
    xQueuePosition = xQueueCreate( 1, sizeof(uint16_t));
    xQueueThrottle = xQueueCreate( 1, sizeof(uint16_t));
    xQueueSlope = xQueueCreate( 1, sizeof(uint16_t));

    vTaskStartScheduler();  /* Start the scheduler. */

    return 0;
}
/*-----------------------------------------------------------*/
