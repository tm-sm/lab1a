#include <stdio.h>
#include "bsp.h"

/*************************************************************/

/**
 * @brief Main function.
 *
 * @return int
 */
int main()
{
    BSP_Init();             /* Initialize all components on the lab-kit. */


    while (true) {
        // init fresh state
        BSP_SetLED(LED_RED, 0);
        BSP_SetLED(LED_GREEN, 0);
        BSP_SetLED(LED_YELLOW, 0);


        BSP_SetLED(LED_RED, 1);
        sleep_ms(3000);
        BSP_SetLED(LED_YELLOW, 1);
        sleep_ms(1000);
        BSP_SetLED(LED_YELLOW, 0);
        BSP_SetLED(LED_RED, 0);
        BSP_SetLED(LED_GREEN, 1);
        sleep_ms(3000);
        BSP_SetLED(LED_GREEN, 0);
        BSP_SetLED(LED_YELLOW, 1);
        sleep_ms(1000);
    }
}
/*-----------------------------------------------------------*/
