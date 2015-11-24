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

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

/* Include D2XX header*/
#include <ftd2xx.h>
#ifndef _COMMON_CODE
#define _COMMON_CODE

#include "settings.h"

BYTE byOutputBuffer[1024 * 1024];  // Buffer to hold MPSSE commands and data
                         //     to be sent to the FT2232H
BYTE byInputBuffer[1024 * 1024];   // Buffer to hold data read from the FT2232H
DWORD dwNumBytesToSend = 0;  // Index to the output buffer
DWORD dwNumBytesSent = 0;  // Count of actual bytes sent - used with FT_Write
DWORD dwNumBytesToRead = 0;  // Number of bytes available to read
                            //     in the driver's input buffer
DWORD dwNumBytesRead = 0;  // Count of actual bytes read - used with FT_Read


/**
 * @brief Open an FTDI port, returning a handle.
 *
 * @param id_string The serial number or description (depending on
 *                  FTDI_USE_DESCRIPTION) of the FTDI device
 * @param pftHandle Pointer to where to store the handle to the FTDI device.
 *
 * @returns Returns 0 on success, -1 on failure
 */
static FT_STATUS getDeviceHandle(char *id_string, FT_HANDLE *pftHandle) {
    FT_STATUS ftStatus;
    unsigned int numDevs;
    FT_DEVICE_LIST_INFO_NODE *devInfo;
    int i;


    // create the device information list
    ftStatus = FT_CreateDeviceInfoList(&numDevs);
    if (ftStatus == FT_OK) {
        printf("Number of devices is %d\n",numDevs);
    }

    //
    // allocate storage for list based on numDevs
    //
    devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);

    //
    // get the device information list
    //
    ftStatus = FT_GetDeviceInfoList(devInfo,&numDevs);
    if (ftStatus == FT_OK) {
#if 0
  #ifdef FTDI_USE_DESCRIPTION
            printf("Looking for device id '%s'\n", id_string);
  #else
            printf("Looking for device s/n '%s'\n", id_string);
  #endif
        for (i = 0; i < numDevs; i++) {
            printf(" %2d: Flags %x, Type %x, ID %x, LocId %x, "
                   "s/n '%s', desc, '%s', ftHandle %p\n",
                   i, devInfo[i].Flags, devInfo[i].Type, devInfo[i].ID,
                   devInfo[i].LocId, devInfo[i].SerialNumber,
                   devInfo[i].Description, devInfo[i].ftHandle);
        }
#endif
        for (i = 0; i < numDevs; i++) {
#ifdef FTDI_USE_DESCRIPTION
            if (strcmp(devInfo[i].Description, id_string) == 0) {
                printf("Found FTDI device id '%s'\n", id_string);
                break;
#else
            if (strcmp(devInfo[i].SerialNumber, id_string) == 0) {
                printf("Found FTDI device s/n '%s'\n", id_string);
                break;
#endif
            }
        }
        free(devInfo);
        if (i == numDevs) {
#ifdef FTDI_USE_DESCRIPTION
            printf("Failed to find FTFI device id '%s'\n", id_string);
#else
            printf("Failed to find FTDI device s/n '%s'\n", id_string);
#endif
            return FT_DEVICE_NOT_FOUND;
        }

        ftStatus = FT_Open(i, pftHandle);
    }
    return ftStatus;
}


/**
 * @brief Configure an FTDI port (specified by serial No), returning a handle.
 *
 * @param id_string The serial number or description (depending on
 *                  FTDI_USE_DESCRIPTION) of the FTDI device
 * @param pftHandle Pointer to where to store the handle to the FTDI device.
 *
 * @returns Returns 0 on success, -1 on failure
 */
FT_STATUS mpsse_init(char *id_string, FT_HANDLE *pftHandle) {
    FT_HANDLE ftHandle;
    FT_STATUS ftStatus;

    ftStatus = getDeviceHandle(id_string, &ftHandle);

    if (ftStatus != FT_OK) {
        return ftStatus;
    }
    // Configure port parameters
    printf("\nConfiguring port for MPSSE use...\n");
    ftStatus |= FT_ResetDevice(ftHandle);        //Reset USB device
    //Purge USB receive buffer first by reading out all old data from FT2232H receive buffer
    ftStatus |= FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
                                                // Get the number of bytes in the FT2232H
                                                // receive buffer
    if ((ftStatus == FT_OK) && (dwNumBytesToRead > 0))
        FT_Read(ftHandle, &byInputBuffer, dwNumBytesToRead, &dwNumBytesRead);        //Read out the data from FT2232H receive buffer
    ftStatus |= FT_SetUSBParameters(ftHandle, 65536, 65535);        //Set USB request transfer sizes to 64K
    ftStatus |= FT_SetChars(ftHandle, false, 0, false, 0);        //Disable event and error characters
    ftStatus |= FT_SetTimeouts(ftHandle, 0, 5000);        //Sets the read and write timeouts in milliseconds
    ftStatus |= FT_SetLatencyTimer(ftHandle, 1);        //Set the latency timer to 1mS (default is 16mS)
    ftStatus |= FT_SetFlowControl(ftHandle, FT_FLOW_RTS_CTS, 0x00, 0x00);        //Turn on flow control to synchronize IN requests
    ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x00);        //Reset controller
    ftStatus |= FT_SetBitMode(ftHandle, 0x0, 0x02);        //Enable MPSSE mode
    if (ftStatus != FT_OK)  {
        printf("Error in initializing the MPSSE %d\n", ftStatus);
        FT_Close(ftHandle);
        return 1;   // Exit with error
    }
    usleep(50 * 1000); // Wait for all the USB stuff to complete and work
    *pftHandle = ftHandle;
    return FT_OK;
}

#endif // _COMMON_CODE
