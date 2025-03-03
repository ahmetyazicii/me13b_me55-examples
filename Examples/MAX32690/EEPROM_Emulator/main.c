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
 * @brief   Example firmware for emulating an EEPROM chip with an I2C interface.
 * @details This example can be used to emulate the behavior of
 *          an EEPROM chip with an I2C interface. See README for
 *          details on how to perform read and write operations
 *          with the device.
 */

/***** Includes *****/
#include <stdbool.h>
#include <stdio.h>
#include "gpio.h"
#include "i2c.h"
#include "include/eeprom.h"
#include "mxc_errors.h"

/***** Definitions *****/
#define EEPROM_I2C MXC_I2C0
#if defined(EvKit_V1)
#define SYNC_PIN_PORT MXC_GPIO2
#define SYNC_PIN_MASK MXC_GPIO_PIN_11
#else
#define SYNC_PIN_PORT MXC_GPIO1
#define SYNC_PIN_MASK MXC_GPIO_PIN_11
#endif

/***** Functions *****/
int main(void)
{
    int err;
    printf("\n********************  EEPROM Emulator Demo *******************\n");

    mxc_gpio_cfg_t sync_pin;
    sync_pin.port = SYNC_PIN_PORT;
    sync_pin.mask = SYNC_PIN_MASK;

    // Initialize EEPROM Emulator
    if ((err = eeprom_init(EEPROM_I2C, sync_pin)) != E_NO_ERROR) {
        printf("Failed to initialize EEPROM Emulator!\n");
        return err;
    }

    while (1) {
        // Start next slave transaction
        eeprom_prep_for_txn();

        // Wait for slave transaction to finish
        while (!eeprom_txn_done) {}
    }
}
