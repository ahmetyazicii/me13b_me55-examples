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
* @file console.h
* @brief Serial console header file
*****************************************************************************/

#ifndef EXAMPLES_MAX78002_IMGCAPTURE_INCLUDE_CONSOLE_H_
#define EXAMPLES_MAX78002_IMGCAPTURE_INCLUDE_CONSOLE_H_
#include "uart.h"
#include "board.h"
#include "example_config.h"

#define SERIAL_BUFFER_SIZE 256
#define CON_BAUD 921600 // UART baudrate used for sending data to PC

#ifdef SD
#include "sd.h"
#endif

typedef enum {
    CMD_UNKNOWN = -1,
    CMD_HELP = 0,
    CMD_RESET,
    CMD_CAPTURE,
    CMD_IMGRES,
    CMD_STREAM,
    CMD_SETREG,
    CMD_GETREG,
#ifdef CAMERA_BAYER
    CMD_SETDEBAYER,
#endif
#ifdef SD
    CMD_SD_MOUNT,
    CMD_SD_UNMOUNT,
    CMD_SD_CWD,
    CMD_SD_CD,
    CMD_SD_LS,
    CMD_SD_MKDIR,
    CMD_SD_RM,
    CMD_SD_TOUCH,
    CMD_SD_WRITE,
    CMD_SD_CAT,
    CMD_SD_SNAP
#endif
} cmd_t;

extern char *cmd_table[];
extern char *help_table[];

static mxc_uart_regs_t *Con_Uart = MXC_UART_GET_UART(CONSOLE_UART);
extern char g_serial_buffer[SERIAL_BUFFER_SIZE];
extern int g_buffer_index;
extern int g_num_commands;

int MXC_UART_WriteBytes(mxc_uart_regs_t *uart, const uint8_t *bytes, int len);

int console_init();
int send_msg(const char *msg);
int recv_msg(char *buffer);
int recv_cmd(cmd_t *out_cmd);
void clear_serial_buffer(void);
void print_help(void);

#ifdef SD
// Supporting function for use with f_forward (http://elm-chan.org/fsw/ff/doc/forward.html)
// Streams fatFS bytes to the UART TX FIFO
UINT out_stream(const BYTE *p, UINT btf);
#endif

#endif // EXAMPLES_MAX78002_IMGCAPTURE_INCLUDE_CONSOLE_H_
