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

/**
 * @file        main.c
 * @brief       Demonstrates the alarm functionality of the Real-Time Clock (RTC)
 * @details     The RTC is enabled and the sub-second alarm set to trigger every
 * 250 ms. P2.25 (LED0) is toggled each time the sub-second alarm triggers.  The
 *              time-of-day alarm is set to 2 seconds.  When the time-of-day
 * alarm triggers, the rate of the sub-second alarm is switched to 500 ms.  The
 *              time-of-day alarm is then rearmed for another 2 sec.  Pressing
 * SW2 will output the current value of the RTC to the console UART.
 */

/***** Includes *****/
#include <stdint.h>
#include <stdio.h>

#include "board.h"
#include "led.h"
#include "lp.h"
#include "mxc_delay.h"
#include "mxc_device.h"
#include "nvic_table.h"
#include "rtc.h"
#include "simo_regs.h"
#include "trimsir_regs.h"
#include "uart.h"

/***** Definitions *****/
#define LED_TODA LED_GREEN
#define TIME_OF_DAY_SEC 7

#define MSEC_TO_RSSA(x) \
    (0 - ((x * 4096) /  \
          1000)) // Converts a time in milleseconds to the equivalent RSSA register value.
#define SECS_PER_MIN 60
#define SECS_PER_HR (60 * SECS_PER_MIN)
#define SECS_PER_DAY (24 * SECS_PER_HR)

/***** Globals *****/

/***** Functions *****/
// *****************************************************************************
void RTC_IRQHandler(void) {}

// *****************************************************************************
void rescheduleAlarm()
{
    uint32_t time;
    int flags = MXC_RTC_GetFlags();

    if (flags & MXC_F_RTC_CTRL_TOD_ALARM) { // Check for TOD alarm flag
        MXC_RTC_ClearFlags(MXC_F_RTC_CTRL_TOD_ALARM);

        MXC_RTC_GetSeconds(&time); // Get Current time (s)

        while (MXC_RTC_DisableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) {}
        // Disable interrupt while re-arming RTC alarm

        if (MXC_RTC_SetTimeofdayAlarm(time + TIME_OF_DAY_SEC) !=
            E_NO_ERROR) { // Reset TOD alarm for TIME_OF_DAY_SEC in the future
            /* Handle Error */
        }

        while (MXC_RTC_EnableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) {}
        // Re-enable TOD alarm interrupt
    }

    MXC_LP_EnableRTCAlarmWakeup(); // Enable RTC as a wakeup source from low poer modes
}

// *****************************************************************************
void printTime()
{
    int day, hr, min;
    uint32_t sec;

    MXC_RTC_GetSeconds(&sec); // Get current time

    day = sec / SECS_PER_DAY;
    sec -= day * SECS_PER_DAY;

    hr = sec / SECS_PER_HR;
    sec -= hr * SECS_PER_HR;

    min = sec / SECS_PER_MIN;
    sec -= min * SECS_PER_MIN;

    printf("\nCurrent Time (dd:hh:mm:ss): %02d:%02d:%02d:%02d\n\n", day, hr, min,
           sec); // Print current time
}

// *****************************************************************************
int configureRTC()
{
    int rtcTrim;
    MXC_Delay(MXC_DELAY_SEC(2)); // Delay to give debugger a window to connect

    /* Switch the system clock to the 32 MHz oscillator */
    MXC_SYS_ClockSourceEnable(MXC_SYS_CLOCK_ERFO);
    MXC_SYS_SetClockDiv(MXC_SYS_CLOCK_DIV_1);
    SystemCoreClockUpdate();

    printf("\n\n***************** RTC Wake from Backup Example *****************\n\n");
    printf("The time-of-day alarm is set to wake the device every %d seconds.\n", TIME_OF_DAY_SEC);
    printf("When the alarm goes off it will print the current time to the console.\n\n");

    if (MXC_RTC_Init(0, 0) != E_NO_ERROR) { // Initialize RTC
        printf("Failed RTC Initialization\n");
        printf("Example Failed\n");
        while (1) {}
    }

    if (MXC_RTC_Start() != E_NO_ERROR) { // Start RTC
        printf("Failed RTC_Start\n");
        printf("Example Failed\n");
        while (1) {}
    }

    printf("RTC started\n");

    NVIC_DisableIRQ(RTC_IRQn);
    rtcTrim = MXC_RTC_TrimCrystal(); // Trim RTC
    if (rtcTrim < 0) {
        printf("Error trimming RTC %d\n", rtcTrim);
    } else {
        printf("RTC Trimmed to %d Hz\n", rtcTrim);
        printf("MXC_TRIMSIR->rtc = 0x%x\n", MXC_TRIMSIR->rtc);
    }

    MXC_RTC_DisableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE | // Reset interrupt state
                       MXC_F_RTC_CTRL_SSEC_ALARM_IE | MXC_F_RTC_CTRL_RDY_IE);
    MXC_RTC_ClearFlags(MXC_RTC_GetFlags());
    NVIC_EnableIRQ(RTC_IRQn);

    if (MXC_RTC_SetTimeofdayAlarm(TIME_OF_DAY_SEC) != E_NO_ERROR) { // Arm TOD alarm
        printf("Failed RTC_SetTimeofdayAlarm\n");
        printf("Example Failed\n");
        while (1) {}
    }

    if (MXC_RTC_EnableInt(MXC_F_RTC_CTRL_TOD_ALARM_IE) == E_BUSY) { // Enable TOD interrupt
        return E_BUSY;
    }

    if (MXC_RTC_Start() != E_NO_ERROR) { // Re-start RTC
        printf("Failed RTC_Start\n");
        printf("Example Failed\n");
        while (1) {}
    }

    return E_NO_ERROR;
}

// *****************************************************************************
int main(void)
{
    if (MXC_PWRSEQ->lppwst !=
        MXC_F_PWRSEQ_LPPWST_BACKUP) { // Check whether the wakeup source is RTC
        if (configureRTC() != E_NO_ERROR) { // System start/restart
            printf("Example Failed\n");
            while (1) {}
        }
    } else {
        LED_On(LED_TODA); // RTC alarm fired off. Perform periodic task here
        printTime();
    }

    rescheduleAlarm(); // Re-arm RTC TOD alarm

    MXC_Delay(MXC_DELAY_SEC(1));
    LED_Off(LED_TODA);

    while (MXC_UART_ReadyForSleep(MXC_UART_GET_UART(CONSOLE_UART)) != E_NO_ERROR) {}

    MXC_LP_EnterBackupMode(); // Enter a backup mode
}
