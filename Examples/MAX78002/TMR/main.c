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
 * @file    main.c
 * @brief   Timer example
 * @details PWM Timer        - Outputs a PWM signal (2Hz, 30% duty cycle) on 3.7
 *          Continuous Timer - Outputs a continuous 1s timer on LED0 (GPIO toggles every 500s)
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include "mxc_device.h"
#include "mxc_sys.h"
#include "nvic_table.h"
#include "lpgcr_regs.h"
#include "gcr_regs.h"
#include "pwrseq_regs.h"
#include "mxc.h"
#include "board.h"
#include "lp.h"

/***** Definitions *****/
#define DEEPSLEEP_MODE // Select between SLEEP_MODE and DEEPSLEEP_MODE for LPTIMER

#define PB2 1

// Parameters for PWM output
#define OST_CLOCK_SOURCE MXC_TMR_32K_CLK // \ref mxc_tmr_clock_t
#define PWM_CLOCK_SOURCE MXC_TMR_APB_CLK // \ref mxc_tmr_clock_t
#define CONT_CLOCK_SOURCE MXC_TMR_8M_CLK // \ref mxc_tmr_clock_t

// Parameters for Continuous timer
#define OST_FREQ 1 // (Hz)
#define OST_TIMER MXC_TMR4 // Can be MXC_TMR0 through MXC_TMR5

#define FREQ 1000 // (Hz)
#define DUTY_CYCLE 50 // (%)
#define PWM_TIMER MXC_TMR0 // Changing this will change the output pin

// Parameters for Continuous timer
#define CONT_FREQ 2 // (Hz)
#define CONT_TIMER MXC_TMR1 // Can be MXC_TMR0 through MXC_TMR5

// Check Frequency bounds
#if (FREQ == 0)
#error "Frequency cannot be 0."
#elif (FREQ > 100000)
#error "Frequency cannot be over 100000."
#endif

// Check duty cycle bounds
#if (DUTY_CYCLE < 0) || (DUTY_CYCLE > 100)
#error "Duty Cycle must be between 0 and 100."
#endif

/***** Functions *****/
void PWMTimer()
{
    // Declare variables
    mxc_tmr_cfg_t tmr; // to configure timer
    unsigned int periodTicks = MXC_TMR_GetPeriod(PWM_TIMER, PWM_CLOCK_SOURCE, 16, FREQ);
    unsigned int dutyTicks = periodTicks * DUTY_CYCLE / 100;

    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the pre-scale value
    3. Set polarity, PWM parameters
    4. Configure the timer for PWM mode
    5. Enable Timer
    */

    MXC_TMR_Shutdown(PWM_TIMER);

    tmr.pres = TMR_PRES_16;
    tmr.mode = TMR_MODE_PWM;
    tmr.bitMode = TMR_BIT_MODE_32;
    tmr.clock = PWM_CLOCK_SOURCE;
    tmr.cmp_cnt = periodTicks;
    tmr.pol = 1;

    if (MXC_TMR_Init(PWM_TIMER, &tmr, true) != E_NO_ERROR) {
        printf("Failed PWM timer Initialization.\n");
        return;
    }

    if (MXC_TMR_SetPWM(PWM_TIMER, dutyTicks) != E_NO_ERROR) {
        printf("Failed TMR_PWMConfig.\n");
        return;
    }

    MXC_TMR_Start(PWM_TIMER);

    printf("PWM started.\n\n");
}

// Toggles GPIO when continuous timer repeats
void ContinuousTimerHandler()
{
    // Clear interrupt
    MXC_TMR_ClearFlags(CONT_TIMER);
    MXC_GPIO_OutToggle(led_pin[0].port, led_pin[0].mask);
}

void ContinuousTimer()
{
    // Declare variables
    mxc_tmr_cfg_t tmr;
    uint32_t periodTicks = MXC_TMR_GetPeriod(CONT_TIMER, CONT_CLOCK_SOURCE, 128, CONT_FREQ);

    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the prescale value
    3  Configure the timer for continuous mode
    4. Set polarity, timer parameters
    5. Enable Timer
    */

    MXC_TMR_Shutdown(CONT_TIMER);

    tmr.pres = TMR_PRES_128;
    tmr.mode = TMR_MODE_CONTINUOUS;
    tmr.bitMode = TMR_BIT_MODE_16B;
    tmr.clock = CONT_CLOCK_SOURCE;
    tmr.cmp_cnt = periodTicks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    if (MXC_TMR_Init(CONT_TIMER, &tmr, true) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        return;
    }

    printf("Continuous timer started.\n\n");
}

void OneshotTimerHandler()
{
    // Clear interrupt
    MXC_TMR_ClearFlags(OST_TIMER);

    // Clear interrupt
    if (MXC_TMR4->wkfl & MXC_F_TMR_WKFL_A) {
        MXC_TMR4->wkfl = MXC_F_TMR_WKFL_A;
        MXC_GPIO_OutToggle(led_pin[1].port, led_pin[1].mask);
        MXC_GPIO_OutToggle(led_pin[1].port, led_pin[1].mask);
        MXC_GPIO_OutToggle(led_pin[1].port, led_pin[1].mask);
    }
}

void OneshotTimer()
{
    // Declare variables
    mxc_tmr_cfg_t tmr;
    uint32_t periodTicks = MXC_TMR_GetPeriod(OST_TIMER, OST_CLOCK_SOURCE, 128, OST_FREQ);
    /*
    Steps for configuring a timer for PWM mode:
    1. Disable the timer
    2. Set the prescale value
    3  Configure the timer for continuous mode
    4. Set polarity, timer parameters
    5. Enable Timer
    */

    MXC_TMR_Shutdown(OST_TIMER);

    tmr.pres = TMR_PRES_128;
    tmr.mode = TMR_MODE_ONESHOT;
    tmr.bitMode = TMR_BIT_MODE_32;
    tmr.clock = OST_CLOCK_SOURCE;
    tmr.cmp_cnt = periodTicks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    if (MXC_TMR_Init(OST_TIMER, &tmr, true) != E_NO_ERROR) {
        printf("Failed Continuous timer Initialization.\n");
        return;
    }

    MXC_TMR_EnableInt(OST_TIMER);

    // Enable wkup source in Poower seq register
    MXC_LP_EnableTimerWakeup(OST_TIMER);
    // Enable Timer wake-up source
    MXC_TMR_EnableWakeup(OST_TIMER, &tmr);

    printf("Oneshot timer started.\n\n");

    MXC_TMR_Start(OST_TIMER);
}

void PB1Handler()
{
    PWMTimer();

    MXC_NVIC_SetVector(TMR1_IRQn, ContinuousTimerHandler);
    NVIC_EnableIRQ(TMR1_IRQn);
    ContinuousTimer();
}

// *****************************************************************************
int main(void)
{
    //Exact timer operations can be found in tmr_utils.c

    printf("\n************************** Timer Example **************************\n\n");
    printf("1. A oneshot mode timer, Timer 4 (low-power timer) is used to create an\n");
    printf("   interrupt at a freq of %d Hz. LED1 (Port 2.5) will toggle when the\n", OST_FREQ);
    printf("   interrupt occurs.\n\n");
    printf("2. Timer 0 is used to output a PWM signal on Port 0.2.\n");
    printf("   The PWM frequency is %d Hz and the duty cycle is %d%%.\n\n", FREQ, DUTY_CYCLE);
    printf("3. Timer 1 is configured as 16-bit timer used in continuous mode\n");
    printf("   which is used to create an interrupt at freq of %d Hz.\n", CONT_FREQ);
    printf("   LED0 (Port 2.4) will toggle when the interrupt occurs.\n\n");
    printf("Push PB1 to start the PWM and continuous timer and PB2 to start lptimer in oneshot "
           "mode.\n\n");

    PB_RegisterCallback(0, (pb_callback)PB1Handler);

    while (1) {
        if (MXC_GPIO_InGet(pb_pin[PB2].port, pb_pin[PB2].mask) == 0) {
            MXC_NVIC_SetVector(TMR4_IRQn, OneshotTimerHandler);
            NVIC_EnableIRQ(TMR4_IRQn);

            OneshotTimer();

#ifdef SLEEP_MODE
            MXC_LP_EnterSleepMode();

#else
            MXC_MCR->ctrl |= MXC_F_MCR_CTRL_ERTCO_EN; // Enabled for deep sleep mode
            MXC_LP_ClearWakeStatus();
            MXC_GCR->pm |= MXC_S_GCR_PM_MODE_UPM; // upm mode
#endif
        }
    }

    return 0;
}
