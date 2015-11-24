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

#include "common.h"
#include "gpio.h"

FT_STATUS gpio_control(FT_HANDLE ftHandle, unsigned int dir, unsigned int set, unsigned int clr) {
    FT_STATUS ftStatus;
    // Read From GPIO low byte
    byOutputBuffer[dwNumBytesToSend++] = 0x81;       // Get data bits - returns state of pins,
                                                     // either input or output
                                                     // on low byte of MPSSE
    ftStatus = FT_Write(ftHandle, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);        // Read the low GPIO byte
    dwNumBytesToSend = 0;   // Reset output buffer pointer
    usleep(200 * 1000);    // Wait for data to be transmitted and status
                  // to be returned by the device driver
                  // - see latency timer above
    // Check the receive buffer - there should be one byte
    ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);        // Get the number of bytes in the
                                                                      // FT2232H receive buffer
    ftStatus |= FT_Read(ftHandle, &byInputBuffer, dwNumBytesToRead, &dwNumBytesRead);
    if ((ftStatus != FT_OK) & (dwNumBytesToRead != 1))  {
        printf("Error - GPIO cannot be read %d\n", ftStatus);
        FT_SetBitMode(ftHandle, 0x0, 0x00);       // Reset the port to disable MPSSE
        FT_Close(ftHandle);  // Close the USB port   return 1;
                             // Exit with error
        return ftStatus;
    }
                                                                  // valid byte at location 0
    // Modify the GPIO data and write it back
    byOutputBuffer[dwNumBytesToSend++] = 0x80;       // Set data bits low-byte of MPSSE port
    printf("0x%x 0x%x 0x%x 0x%x\r\n", byInputBuffer[0], clr, set, (byInputBuffer[0] & ~clr) | set);
    byOutputBuffer[dwNumBytesToSend++] = (byInputBuffer[0] & ~clr) | set;
    byOutputBuffer[dwNumBytesToSend++] = dir;       // Direction config is still needed for each GPIO write
    ftStatus = FT_Write(ftHandle, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);        // Send off the low GPIO config commands
    dwNumBytesToSend = 0;   // Reset output buffer pointer
    usleep(200 * 1000);    // Wait for data to be transmitted and status
                         // to be returned by the device driver
                         // - see latency timer above
    return ftStatus;
}


