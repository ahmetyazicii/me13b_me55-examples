/**
 * @file        main.c
 * @brief       I2S Receiver Example
 * @details
 * @note
 */

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

/***** Includes *****/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "board.h"
#include "icc.h"
#include "i2s.h"
#include "led.h"
#include "mxc_delay.h"
#include "mxc_device.h"
#include "mxc_sys.h"
#include "nvic_table.h"

#ifdef BOARD_FTHR_REVA
#include "max20303.h"
#endif

#define I2S_RX_BUFFER_SIZE 256
int32_t i2s_rx_buffer[I2S_RX_BUFFER_SIZE];

/***** Global Data *****/
volatile uint8_t i2s_flag = 0;
uint8_t recv_data = 0;

void i2s_isr(void)
{
    i2s_flag = 1;
    /* Clear I2S interrupt flag */
    MXC_I2S_ClearFlags(MXC_F_I2S_INTFL_RX_THD_CH0);
}

/*****************************************************************/
int main()
{
    int32_t err;
    mxc_i2s_req_t req;
    uint32_t rx_size;
    int32_t *buf_current, *buf_start, *buf_end;

    /* Enable cache */
    MXC_ICC_Enable(MXC_ICC0);

    /* Set system clock to 100 MHz */
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    /* Jumper J20 (I2S CLK SEL) needs to be installed to INT position to provide 12.288MHz clock from on-board oscillator */
    printf("\n***** I2S Receiver Example *****\n");

/* Initialize microphone on the Featherboard */
#ifdef BOARD_FTHR_REVA
    if (max20303_init(MXC_I2C1) != E_NO_ERROR) {
        printf("Unable to initialize I2C interface to commonicate with PMIC!\n");
        while (1) {}
    }

    if (max20303_mic_power(1) != E_NO_ERROR) {
        printf("Unable to turn on microphone!\n");
        while (1) {}
    }

    MXC_Delay(MXC_DELAY_MSEC(200));

    printf("\nMicrophone enabled!\n");
#endif

    /* Initialize I2S RX buffer */
    memset(i2s_rx_buffer, 0, sizeof(i2s_rx_buffer));

    /* Configure I2S interface parameters */
    req.wordSize = MXC_I2S_WSIZE_WORD;
    req.sampleSize = MXC_I2S_SAMPLESIZE_THIRTYTWO;
    req.bitsWord = 32;
    req.adjust = MXC_I2S_ADJUST_LEFT;
    req.justify = MXC_I2S_MSB_JUSTIFY;
    req.wsPolarity = MXC_I2S_POL_NORMAL;
    req.channelMode = MXC_I2S_INTERNAL_SCK_WS_0;
    /* Get only left channel data from on-board microphone. Right channel samples are zeros */
    req.stereoMode = MXC_I2S_MONO_LEFT_CH;
    req.bitOrder = MXC_I2S_MSB_FIRST;
    /* I2S clock = 12.288MHz / (2*(req.clkdiv + 1)) = 1.024 MHz */
    /* I2S sample rate = 1.024 MHz/64 = 16kHz */
    req.clkdiv = 5;
    req.rawData = NULL;
    req.txData = NULL;
    req.rxData = i2s_rx_buffer;
    req.length = I2S_RX_BUFFER_SIZE;

    if ((err = MXC_I2S_Init(&req)) != E_NO_ERROR) {
        printf("\nError in I2S_Init: %d\n", err);

        while (1) {}
    }

    /* Set I2S RX FIFO threshold to generate interrupt */
    MXC_I2S_SetRXThreshold(4);
    MXC_NVIC_SetVector(I2S_IRQn, i2s_isr);
    NVIC_EnableIRQ(I2S_IRQn);
    /* Enable RX FIFO Threshold Interrupt */
    MXC_I2S_EnableInt(MXC_F_I2S_INTEN_RX_THD_CH0);
    MXC_I2S_RXEnable();

    buf_start = &i2s_rx_buffer[0];
    buf_end = &i2s_rx_buffer[I2S_RX_BUFFER_SIZE - 1];
    buf_current = buf_start;

    while (1) {
        /* Wait for I2S interrupt */
        while (i2s_flag == 0) {}

        /* Clear flag */
        i2s_flag = 0;
        /* Read number of samples in I2S RX FIFO */
        rx_size = MXC_I2S->dmach0 >> MXC_F_I2S_DMACH0_RX_LVL_POS;
        // printf("%d ", rx_size);

        while (rx_size--) {
            /* Copy captured microphone sample into i2s_rx_buffer. The actual value is 18 MSB of 32-bit word */
            *buf_current++ = ((int32_t)MXC_I2S->fifoch0) >> 14;

            if (buf_current > buf_end) {
                buf_current = buf_start;
            }
        }

        if (!recv_data && *(buf_current - 1) != 0) {
            printf("Receiving microphone data!\n");
            recv_data = 1;
        }
    }

    return 0;
}
