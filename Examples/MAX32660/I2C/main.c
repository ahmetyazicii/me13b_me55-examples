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
 * @brief       I2C Master-Slave Transaction Demo
 * @details     This example uses the I2C Master to read/write from/to the I2C Slave. 
 *              For this example, user must connect I2C Master SCL pin to I2C Slave SCL 
 *              pin and I2C Master SDA pin to I2C Slave SDA pin. User must also connect
 *              the pull-up jumpers to the proper I/O voltage. Refer UART messages for
 *              more information.
 * @note        Other devices on the EvKit might be using the same I2C bus. While 
 *              combining this example with other examples, make sure I2C pins are not
 *              being used in other examples of any other function (like GPIO).
 */

/***** Includes *****/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc_errors.h"
#include "nvic_table.h"
#include "i2c.h"
#include "dma.h"

/***** Definitions *****/

#define MASTERDMA //Comment this line out if standard I2C transaction is required

#define I2C_MASTER MXC_I2C0
#define I2C_SLAVE MXC_I2C1

#define I2C_FREQ 100000
#define I2C_SLAVE_ADDR (0x51)
#define I2C_BYTES 255

/***** Globals *****/

static uint8_t Stxdata[I2C_BYTES];
static uint8_t Srxdata[I2C_BYTES];
static uint8_t txdata[I2C_BYTES];
static uint8_t rxdata[I2C_BYTES];
int8_t DMA_TX_CH;
int8_t DMA_RX_CH;
volatile int I2C_FLAG;
volatile int txnum = 0;
volatile int txcnt = 0;
volatile int rxnum = 0;

/***** Functions *****/

//Slave interrupt handler
void I2C1_IRQHandler(void)
{
    MXC_I2C_AsyncHandler(I2C_SLAVE);
}

void DMA_TX_IRQHandler(void)
{
    MXC_DMA_Handler();
}

void DMA_RX_IRQHandler(void)
{
    MXC_DMA_Handler();
}

//I2C callback function
void I2C_Callback(mxc_i2c_req_t *req, int error)
{
    I2C_FLAG = error;
}

int slaveHandler(mxc_i2c_regs_t *i2c, mxc_i2c_slave_event_t event, void *data)
{
    switch (event) {
    case MXC_I2C_EVT_MASTER_WR:
        // If we're being written to
        // Clear bytes written
        rxnum = 0;
        break;

    case MXC_I2C_EVT_MASTER_RD:
        // Serve as a 16 byte loopback, returning data*2
        txnum = I2C_BYTES;
        txcnt = 0;
        i2c->intfl0 = MXC_F_I2C_INTFL0_TXLOI | MXC_F_I2C_INTFL0_AMI;
        break;

    case MXC_I2C_EVT_RX_THRESH:
    case MXC_I2C_EVT_OVERFLOW:
        rxnum += MXC_I2C_ReadRXFIFO(i2c, &Srxdata[rxnum], MXC_I2C_GetRXFIFOAvailable(i2c));
        if (rxnum == I2C_BYTES) {
            i2c->inten0 |= MXC_F_I2C_INTEN0_AMIE;
        }

        break;

    case MXC_I2C_EVT_TX_THRESH:
    case MXC_I2C_EVT_UNDERFLOW:
        // Write as much data as possible into TX FIFO
        // Unless we're at the end of the transaction (only write what's needed)
        if (txcnt >= txnum) {
            break;
        }

        int num = MXC_I2C_GetTXFIFOAvailable(i2c);
        num = (num > (txnum - txcnt)) ? (txnum - txcnt) : num;
        txcnt += MXC_I2C_WriteTXFIFO(i2c, &Stxdata[txcnt], num);
        break;

    default:
        if (*((int *)data) == E_COMM_ERR) {
            printf("I2C Slave Error!\n");
            printf("i2c->intfl0 = 0x%08x\n", i2c->intfl0);
            printf("i2c->status = 0x%08x\n", i2c->status);
            I2C_Callback(NULL, E_COMM_ERR);
            return 1;

        } else if (*((int *)data) == E_NO_ERROR) {
            rxnum += MXC_I2C_ReadRXFIFO(i2c, &Srxdata[rxnum], MXC_I2C_GetRXFIFOAvailable(i2c));
            I2C_Callback(NULL, E_NO_ERROR);
            return 1;
        }
    }

    return 0;
}

//Prints out human-friendly format to read txdata and rxdata
void printData(void)
{
    int i;

    printf("\n-->TxData: ");

    for (i = 0; i < sizeof(txdata); ++i) {
        printf("%02x ", txdata[i]);
    }

    printf("\n\n-->RxData: ");

    for (i = 0; i < sizeof(rxdata); ++i) {
        printf("%02x ", rxdata[i]);
    }

    printf("\n");

    return;
}

//Compare data to see if they are the same
int verifyData(void)
{
    int i, fails = 0;

    for (i = 0; i < I2C_BYTES; ++i) {
        if (txdata[i] != rxdata[i]) {
            ++fails;
        }
    }

    if (fails > 0) {
        return E_FAIL;
    }

    return E_NO_ERROR;
}

// *****************************************************************************
int main()
{
    printf("\n******** I2C Master-Slave Transaction Demo *********\n");
    printf("\nThis example uses one I2C peripheral as a master to\n");
    printf("read and write to another I2C which acts as a slave.\n");

    printf("\nYou will need to connect P0.8->P0.2 (SCL) and\n");
    printf("P0.9->P0.3 (SDA).\n");

    int error, i = 0;

    //Setup the I2CM
    error = MXC_I2C_Init(I2C_MASTER, 1, 0);
    if (error != E_NO_ERROR) {
        printf("Failed master.\n");
        return error;
    }

#ifdef MASTERDMA
    //Setup the I2CM DMA
    error = MXC_I2C_DMA_Init(I2C_MASTER, MXC_DMA, true, true);
    if (error != E_NO_ERROR) {
        printf("Failed DMA master\n");
        return error;
    }
#endif

    printf("\n-->I2C Master Initialization Complete");

    //Setup the I2CS
    error = MXC_I2C_Init(I2C_SLAVE, 0, I2C_SLAVE_ADDR);
    if (error != E_NO_ERROR) {
        printf("Failed slave\n");
        return error;
    }

    printf("\n-->I2C Slave Initialization Complete");

    MXC_NVIC_SetVector(I2C1_IRQn, I2C1_IRQHandler);
    NVIC_EnableIRQ(I2C1_IRQn);
    __enable_irq();

    MXC_I2C_SetFrequency(I2C_MASTER, I2C_FREQ);

    MXC_I2C_SetFrequency(I2C_SLAVE, I2C_FREQ);

    // Initialize test data
    for (i = 0; i < I2C_BYTES; i++) {
        txdata[i] = i;
        rxdata[i] = 0;
        Stxdata[i] = i;
        Srxdata[i] = 0;
    }

    // This will write data to slave
    // Then read data back from slave
    mxc_i2c_req_t reqMaster;
    reqMaster.i2c = I2C_MASTER;
    reqMaster.addr = I2C_SLAVE_ADDR;
    reqMaster.tx_buf = txdata;
    reqMaster.tx_len = I2C_BYTES;
    reqMaster.rx_buf = rxdata;
    reqMaster.rx_len = I2C_BYTES;
    reqMaster.restart = 0;
    reqMaster.callback = I2C_Callback;
    I2C_FLAG = 1;

    printf("\n\n-->Writing data to slave, and reading the data back\n");

    if ((error = MXC_I2C_SlaveTransactionAsync(I2C_SLAVE, slaveHandler)) != 0) {
        printf("Error Starting Slave Transaction %d\n", error);
        return error;
    }

#ifdef MASTERDMA
    DMA_TX_CH = MXC_I2C_DMA_GetTXChannel(I2C_MASTER);
    DMA_RX_CH = MXC_I2C_DMA_GetRXChannel(I2C_MASTER);

    NVIC_EnableIRQ(MXC_DMA_CH_GET_IRQ(DMA_TX_CH));
    NVIC_EnableIRQ(MXC_DMA_CH_GET_IRQ(DMA_RX_CH));

    MXC_NVIC_SetVector(MXC_DMA_CH_GET_IRQ(DMA_TX_CH), DMA_TX_IRQHandler);
    MXC_NVIC_SetVector(MXC_DMA_CH_GET_IRQ(DMA_RX_CH), DMA_RX_IRQHandler);

    if ((error = MXC_I2C_MasterTransactionDMA(&reqMaster)) != 0) {
        printf("Error writing: %d\n", error);
        return error;
    }
#else
    if ((error = MXC_I2C_MasterTransaction(&reqMaster)) != 0) {
        printf("Error writing: %d\n", error);
        return error;
    }
#endif

    while (I2C_FLAG == 1) {}

    printf("\n-->Result: \n");
    printData();
    printf("\n");

    MXC_I2C_Shutdown(I2C_MASTER);
    MXC_I2C_Shutdown(I2C_SLAVE);

    if (verifyData() == E_NO_ERROR) {
        printf("\n-->I2C Transaction Successful\n");
    } else {
        printf("\n-->I2C Transaction Failed\n");
        return E_FAIL;
    }

    return E_NO_ERROR;
}
