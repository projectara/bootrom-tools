/*
 * Copyright (c) 2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "settings.h"
#include "gpio.h"
#include "uart.h"
#include <ftd2xx.h>
#include "reset.h"


#define GPIO_RESET_PULSE    (100 * 1000)

unsigned char buf[1024 * 1024];


/**
 * @brief Assert the reset level on one of the GPIO devices.
 *
 * @param ftHandle The handle to the FTDI GPIO device
 *
 * @returns Returns 0 on success, -1 on failure
 */
int reset_gpio_assert(FT_HANDLE ftHandle) {
    if (gpio_control(ftHandle, 0x01, 0x01, 0) == FT_OK) {
        return 0;
    } else {
        return -1;
    }
}


/**
 * @brief Dessert the reset level on one of the GPIO devices.
 *
 * @param ftHandle The handle to the FTDI GPIO device
 *
 * @returns Returns 0 on success, -1 on failure
 */
int reset_gpio_deassert(FT_HANDLE ftHandle) {
    if (gpio_control(ftHandle, 0x01, 0, 0x01) == FT_OK) {
        return 0;
    } else {
        return -1;
    }
}


/**
 * @brief Generate a reset pulse on one of the GPIO devices.
 *
 * @returns Returns 0 on success, -1 on failure
 */
int reset_haps_pulse(void) {
    FT_HANDLE ftHandle = NULL;
    FT_STATUS ftStatus;
    unsigned int bytesRead;
    int status = 0;

    ftStatus = mpsse_init(MONITOR_ID, &ftHandle);
    if (ftStatus != FT_OK) {
        status = -1;
    } else {
        FT_SetFlowControl(ftHandle, FT_FLOW_NONE, 0x00, 0x00);
        FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
        FT_SetBaudRate(ftHandle, FT_BAUD_230400);

        /* Wait for HAPS ready */
        uart_write(ftHandle, "\r", 1);
        usleep(GPIO_RESET_PULSE);
        do {
            uart_read(ftHandle, buf, &bytesRead);
            buf[bytesRead] = 0;
            //printf("%s\n", buf);
            if (strstr(buf, "HAPS62>") != 0) {
                printf("Got HAPS62>\n");
                break;
            }
        } while(1);

        /* Send the reset commands */
        uart_write(ftHandle, "cfg_reset_set 0\r", 16);
        usleep(GPIO_RESET_PULSE);
        uart_write(ftHandle, "cfg_reset_set 1\r", 16);
        FT_Close(ftHandle);
    }

    return status;
}

