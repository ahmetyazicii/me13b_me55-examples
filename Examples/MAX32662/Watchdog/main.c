/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. (now owned by 
 * Analog Devices, Inc.),
 * Copyright (C) 2023-2024 Analog Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

/*
 * @file    main.c
 * @brief   Demonstrates a watchdog timer in run mode
 *
 * @details When the program starts LED3 blinks three times and stops.
 *          Then LED0 start blinking continuously.
 *          Open a terminal program to see interrupt messages.
 *
 *          SW2: Push SW2 (PB0) to trigger a watchdog reset. This will reset the watchdog before or after
 *               the wait period has expired and trigger an interrupt.
 *          Whether not SW2 will trigger a overflow or underflow event will depend on whether the OVERFLOW compiler
 * definition has been defined.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "nvic_table.h"
#include "board.h"
#include "mxc_sys.h"
#include "wdt.h"
#include "mxc_delay.h"
#include "led.h"
#include "pb.h"

/***** Definitions *****/
#define OVERFLOW
// If this is defined the example will test WDT overflow.
// Comment out the line above and recompile to test WDT underflow.

#ifndef OVERFLOW
#define UNDERFLOW
#endif

/***** Globals *****/
//use push buttons defined in board.h
extern const mxc_gpio_cfg_t pb_pin[];
extern const mxc_gpio_cfg_t led_pin[];

static mxc_wdt_cfg_t cfg;
volatile int pb_int;

// refers to array, do not change constants
#define SW2 0
#define LED 0
/***** Functions *****/

// *****************************************************************************
void watchdogHandler()
{
    MXC_WDT_ClearIntFlag(MXC_WDT0);
    printf("\nWATCHDOG INTERRUPT TRIGGERED! \n");
}

// *****************************************************************************
void WDT_IRQHandler(void)
{
    watchdogHandler();
}
// *****************************************************************************

void SW1_Callback()
{
    pb_int = 1; // Signal to main loop
}

// *****************************************************************************
int main(void)
{
    cfg.mode = MXC_WDT_WINDOWED;
    MXC_WDT_Init(MXC_WDT0, &cfg);

    if (MXC_WDT_GetResetFlag(MXC_WDT0)) {
        uint32_t resetFlags = MXC_WDT_GetResetFlag(MXC_WDT0);

        printf("\nRecovering from watchdog reset...\n");
        if (resetFlags == MXC_F_WDT_CTRL_RST_LATE) {
            printf("Watchdog Reset occured too late (OVERFLOW)\n");
        } else if (resetFlags == MXC_F_WDT_CTRL_RST_EARLY) {
            printf("Watchdog Reset occured too soon (UNDERFLOW)\n");
        }

        MXC_WDT_Disable(MXC_WDT0);
        MXC_WDT_ClearResetFlag(MXC_WDT0);
        MXC_WDT_ClearIntFlag(MXC_WDT0);
        MXC_WDT_EnableReset(MXC_WDT0);
    } else {
        MXC_WDT_Disable(MXC_WDT0);
        cfg.upperResetPeriod = MXC_WDT_PERIOD_2_28;
        cfg.upperIntPeriod = MXC_WDT_PERIOD_2_27;
        cfg.lowerResetPeriod = MXC_WDT_PERIOD_2_24;
        cfg.lowerIntPeriod = MXC_WDT_PERIOD_2_23;
        MXC_WDT_SetResetPeriod(MXC_WDT0, &cfg);
        MXC_WDT_SetIntPeriod(MXC_WDT0, &cfg);
        MXC_WDT_EnableReset(MXC_WDT0);
        MXC_WDT_EnableInt(MXC_WDT0);
        MXC_WDT_Enable(MXC_WDT0);
    }
    NVIC_EnableIRQ(WDT_IRQn);

    printf("\n************** Watchdog Timer Demo ****************\n");
    printf("Watchdog timer is configured in Windowed mode. This example can be compiled\n");
    printf("for two tests: Timer Overflow and Underflow. ");
#ifdef OVERFLOW
    printf("It's currently compiled for timer Overflow.\n");
#else
    printf("It's currently compiled for timer Underflow.\n");
#endif
    printf("\nIt should be noted that triggering the watchdog reset\n");
    printf("will reset the microcontroller.  As such, this\n");
    printf("example runs better without a debugger attached.\n");
    printf("\nPress SW2 (PB0) to create watchdog interrupt and reset.\n\n");

    //Blink LED
    MXC_GPIO_OutClr(led_pin[0].port, led_pin[0].mask);

    //Blink LED three times at startup
    int numBlinks = 3;

    while (numBlinks) {
        MXC_GPIO_OutSet(led_pin[0].port, led_pin[0].mask);
        MXC_Delay(MXC_DELAY_MSEC(100));
        MXC_GPIO_OutClr(led_pin[0].port, led_pin[0].mask);
        MXC_Delay(MXC_DELAY_MSEC(100));
        numBlinks--;
    }

    // Link timeout ISR to SW2.  This enables triggering the timeout.
    pb_int = 0;
    PB_RegisterCallback(SW2, SW1_Callback);

    // Enable Watchdog
    MXC_WDT_Enable(MXC_WDT0);

    while (1) {
        //blink LED0
        MXC_Delay(MXC_DELAY_MSEC(500));
        MXC_GPIO_OutSet(led_pin[0].port, led_pin[0].mask);
        MXC_Delay(MXC_DELAY_MSEC(500));
        MXC_GPIO_OutClr(led_pin[0].port, led_pin[0].mask);

        if (pb_int) {
            // Trigger the compiled timeout condition...

            pb_int = 0;
#ifdef OVERFLOW
            printf("\nHolding to trigger overflow condition...\n");
            while (1) {}
// Let the WDT expire.  "Overflow"
#endif
#ifdef UNDERFLOW
            // Issue a reset before the WDT window.  "Underflow"
            printf("\nFeeding watchdog early to trigger underflow condition...\n");
            MXC_Delay(MXC_DELAY_MSEC(50));
            MXC_WDT_ResetTimer(MXC_WDT0);
            MXC_WDT_ResetTimer(MXC_WDT0); // Double reset sequence guarantees underflow.
#endif
        } else {
            //Feed watchdog
            printf("Feeding watchdog...\n");
            MXC_WDT_ResetTimer(MXC_WDT0);
        }
    }
}
