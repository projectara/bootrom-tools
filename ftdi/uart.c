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

/* Read N bytes from an FTDI serial port into a buffer */
FT_STATUS uart_read(FT_HANDLE ftHandle, unsigned char *buf, unsigned int *bytesRead) {
    FT_STATUS ftStatus;

    while(1) {
        ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
        if (ftStatus != FT_OK || dwNumBytesToRead == 0) continue;
        FT_Read(ftHandle, &byInputBuffer, dwNumBytesToRead, &dwNumBytesRead);
        //printf("read %d bytes %d\n", dwNumBytesRead, dwNumBytesToRead);
        int i, j;
        j = 0;
        for (i=0;i<dwNumBytesRead;i++) {
            //printf("%c(%02X)", byInputBuffer[i], byInputBuffer[i]);
            if (byInputBuffer[i] == 0x02 && i < dwNumBytesRead - 1 && byInputBuffer[i + 1] == 0x60) {
                i++;
                continue;
            }
            buf[j++] = byInputBuffer[i];
        }  
        *bytesRead = j;
        break;
    }
    return ftStatus;
}

#include <sys/time.h>

/* Obtain the current time (seconds+milliseconds+microseconds) in milliseconds */
long get_current_time_in_ms (void)
{
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}


/* Monitor the FTDI serial port queue and write it to stdout */
FT_STATUS uart_print(FT_HANDLE ftHandle) {
    FT_STATUS ftStatus;
    const clock_t start = get_current_time_in_ms();
    bool newline = true;

    while(1) {
        ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
        if (ftStatus != FT_OK || dwNumBytesToRead == 0) continue;
        FT_Read(ftHandle, &byInputBuffer, dwNumBytesToRead, &dwNumBytesRead);
       
        //printf("read %d bytes %d\n", dwNumBytesRead, dwNumBytesToRead);

        int i;
        for (i=0;i<dwNumBytesRead;i++) {
            if (byInputBuffer[i] == 0x02 && i < dwNumBytesRead - 1 && byInputBuffer[i + 1] == 0x60) {
                i++;
                continue;
            }
            if (newline) {
                newline = false;
                //printf("%08ld: ", (get_current_time_in_ms() - start));
            }
            printf("%c", byInputBuffer[i]);
            if (byInputBuffer[i] == '\n') {
                newline = true;
            }
        }
    }
    return ftStatus;
}


/**
 * @brief Monitor the FTDI serial port queue and dribble it to a file.
 *
 * @param ftHandle The handle to the bridge debugserial FTDI device.
 * @param fp The handle log file.
 * @param fp The timeout in seconds.
 *
 * @returns Returns FT_OK on success, other FT_xxx on failure
 */
FT_STATUS uart_dump(FT_HANDLE ftHandle, FILE *fp, long timeout) {
    FT_STATUS ftStatus;
    const clock_t start = get_current_time_in_ms();
    bool newline = true;

    /* Convert the timeout into milliseconds */
    timeout *= 1000;

    while(1) {
        /* 10-sec timeout for detecting end-of-test */
        if ((get_current_time_in_ms() - start) > timeout) {
            return 0;
        }
        ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
        if (ftStatus != FT_OK || dwNumBytesToRead == 0) continue;
        FT_Read(ftHandle, &byInputBuffer, dwNumBytesToRead, &dwNumBytesRead);
       

        int i;
        for (i = 0; i < dwNumBytesRead; i++) {
            char ch;
            /* Skip garbage: "/002`" */
            if (byInputBuffer[i] == 0x02 &&
                i < dwNumBytesRead - 1 &&
                byInputBuffer[i + 1] == 0x60) {
                i++;
                continue;
            }
            if (newline) {
                newline = false;
                //printf("%08ld: ", (get_current_time_in_ms() - start));
            }

            ch = byInputBuffer[i];
#if 0
            printf("%c", byInputBuffer[i]);
            fprintf(fp, "%c", byInputBuffer[i]);
            if (byInputBuffer[i] == '\n') {
                newline = true;
            }
#else
            /* Strip out all control characters except newline and tab */
            if ((ch == '\n') || (ch == '\t') || 
                ((ch >= ' ') && (ch <= '~'))) {
                printf("%c", ch);
                fprintf(fp, "%c", ch);
                if (ch == '\n') {
                    newline = true;
                }
            }
#endif
        }
    }
    return ftStatus;
}

/* Write a buffer to an FTDI serial port */
FT_STATUS uart_write(FT_HANDLE ftHandle, unsigned char *buf, unsigned int bytesToWrite) {
    FT_STATUS ftStatus;

    memcpy(byOutputBuffer, buf, bytesToWrite);
    dwNumBytesToSend = bytesToWrite;
    ftStatus = FT_Write(ftHandle, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);        // Send off the command
    dwNumBytesToSend = 0; 
    return ftStatus;
}



