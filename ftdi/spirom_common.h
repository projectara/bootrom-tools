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

#ifndef _SPIROM_COMMON_H
#define _SPIROM_COMMON_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
/* OS specific libraries */
#ifdef _WIN32
#include <windows.h>
#endif

/* Include D2XX header*/
#include <ftd2xx.h>

/* Include libMPSSE header */
//#include "spirom_libMPSSE_spi.h"
#include "libMPSSE_spi.h"
/******************************************************************************/
/*                           Macro and type defines                           */
/******************************************************************************/
/* Helper macros */

#define APP_CHECK_STATUS(exp) {if(exp!=FT_OK){printf("%s:%d:%s(): status(0x%x) \
!= FT_OK\n",__FILE__, __LINE__, __FUNCTION__,exp);exit(1);}else{;}};
#define CHECK_NULL(exp){if(exp==NULL){printf("%s:%d:%s():  NULL expression \
encountered \n",__FILE__, __LINE__, __FUNCTION__);exit(1);}else{;}};

/* Application specific macro definations */
#define SPI_DEVICE_BUFFER_SIZE        1024 * 1024
#define SPI_WRITE_COMPLETION_RETRY        10
#define START_ADDRESS_EEPROM     0x00 /*read/write start address inside the EEPROM*/
#define END_ADDRESS_EEPROM        0x10
#define RETRY_COUNT_EEPROM        10    /* number of retries if read/write fails */
//#define CHANNEL_TO_OPEN            0    /*0 for first available channel, 1 for next... */
#define SPI_SLAVE_0                0
#define SPI_SLAVE_1                1
#define SPI_SLAVE_2                2
#define DATA_OFFSET                4
#define USE_WRITEREAD            0


/*
 * Global variables:
 */
extern FT_HANDLE ftHandle;
extern uint8 rBuffer[SPI_DEVICE_BUFFER_SIZE];
extern uint8 wBuffer[SPI_DEVICE_BUFFER_SIZE];


/*
 * Function prototypes:
 */

FT_STATUS ReadWrite(int sizeToTransfer, int *sizeTransferred);

void dumprBuffer(char *s);

FT_STATUS WriteEnable(void);

void WaitForWriteDone(void);

FT_STATUS ChipErase(void);


FT_STATUS spi_init(int channelA);

FT_STATUS spi_deinit(void);

#endif /* !_SPIROM_COMMON_H */

