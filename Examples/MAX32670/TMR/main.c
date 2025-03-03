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
#include "gpio.h"
#include "board.h"
#include "tmr.h"
#include "led.h"

/***** Definitions *****/

// Parameters for PWM output
#define CLOCK_SOURCE MXC_TMR_8M_CLK // must be mxc_tmr_clock_t

// Parameters for Continuous timer
#define OST_FREQ 1 // (Hz)
#define OST_TIMER MXC_TMR1 // Can be MXC_TMR0 through MXC_TMR5

#define FREQ 1000 // (Hz)
#define DUTY_CYCLE 50 // (%)
#define PWM_TIMER MXC_TMR2 // must change PWM_PORT and PWM_PIN if changed

// Parameters for Continuous timer
#define CONT_FREQ 4 // (Hz)
#define CONT_TIMER MXC_TMR3 // Can be MXC_TMR0 through MXC_TMR5

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

/***** Globals *****/

/***** Functions *****/

void OneshotTimer(void);
void ContinuousTimer(void);

void PWMTimer(void)
{
    // Declare variables
    mxc_tmr_cfg_t tmr; // to configure timer
    unsigned int periodTicks = MXC_TMR_GetPeriod(PWM_TIMER, CLOCK_SOURCE, 16, FREQ);
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
    tmr.clock = CLOCK_SOURCE;
    tmr.cmp_cnt = periodTicks;
    tmr.pol = 1;

    MXC_TMR_Init(PWM_TIMER, &tmr, true);

    if (MXC_TMR_SetPWM(PWM_TIMER, dutyTicks) != E_NO_ERROR) {
        printf("Failed TMR_PWMConfig.\n");
    }

    MXC_TMR_Start(PWM_TIMER);

    printf("PWM started.\n\n");
}

// Toggles GPIO when continuous timer repeats
void ContinuousTimerHandler(void)
{
    // Clear interrupt
    MXC_TMR_ClearFlags(CONT_TIMER);
    MXC_GPIO_OutToggle(led_pin[0].port, led_pin[0].mask);
}

void ContinuousTimer(void)
{
    // Declare variables
    mxc_tmr_cfg_t tmr;
    uint32_t periodTicks = MXC_TMR_GetPeriod(CONT_TIMER, CLOCK_SOURCE, 128, CONT_FREQ);

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
    tmr.clock = CLOCK_SOURCE;
    tmr.cmp_cnt = periodTicks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    MXC_TMR_Init(CONT_TIMER, &tmr, true);
    MXC_TMR_EnableInt(CONT_TIMER);
    MXC_TMR_Start(CONT_TIMER);

    printf("Continuous timer started.\n\n");
}

void OneshotTimerHandler(void)
{
    // Clear interrupt
    MXC_TMR_ClearFlags(OST_TIMER);
    MXC_GPIO_OutToggle(led_pin[1].port, led_pin[1].mask);
}

void OneshotTimer(void)
{
    // Declare variables
    mxc_tmr_cfg_t tmr;
    uint32_t periodTicks = MXC_TMR_GetPeriod(OST_TIMER, CLOCK_SOURCE, 128, OST_FREQ);

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
    tmr.bitMode = TMR_BIT_MODE_16B;
    tmr.clock = CLOCK_SOURCE;
    tmr.cmp_cnt = periodTicks; //SystemCoreClock*(1/interval_time);
    tmr.pol = 0;

    MXC_TMR_Init(OST_TIMER, &tmr, true);

    printf("Oneshot timer started.\n\n");
}

// *****************************************************************************
int main(void)
{
    //Exact timer operations can be found in tmr_utils.c

    printf("\n************************** Timer Example **************************\n\n");
    printf("1. A continuous mode timer is used to create an interrupt at freq of %d Hz.\n",
           CONT_FREQ);
    printf("   LED0 (Port 0.22) will toggle each time the interrupt occurs.\n\n");
    printf("2. Timer 2 is used to output a PWM signal on Port 0.5.\n");
    printf("   The PWM frequency is %d Hz and the duty cycle is %d%%.\n\n", FREQ, DUTY_CYCLE);
    printf("3. Timer 1 is configured as 16-bit timer used in oneshot mode\n");
    printf("   which is used to create an interrupt at freq of %d Hz.\n", OST_FREQ);
    printf("   LED1 (Port 0.23) will toggle when the interrupt occurs.\n\n");
    PWMTimer();

    MXC_NVIC_SetVector(TMR3_IRQn, ContinuousTimerHandler);
    NVIC_EnableIRQ(TMR3_IRQn);
    ContinuousTimer();

    MXC_NVIC_SetVector(TMR1_IRQn, OneshotTimerHandler);
    NVIC_EnableIRQ(TMR1_IRQn);
    OneshotTimer();

    printf("\nStarted Timer");

    while (1) {}

    return 0;
}
