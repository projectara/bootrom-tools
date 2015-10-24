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
#include <sys/time.h>

#ifndef _UART_H
#define _UART_H


/*
 * Function prototypes:
 */

/* Read N bytes from an FTDI serial port into a buffer */
FT_STATUS uart_read(FT_HANDLE ftHandle, unsigned char *buf,
                    unsigned int *bytesRead);


/*
 * Obtain the current time (seconds+milliseconds+microseconds) in milliseconds
 */
long get_current_time_in_ms (void);


/* Monitor the FTDI serial port queue and write it to stdout */
FT_STATUS uart_print(FT_HANDLE ftHandle);


/* Monitor the FTDI serial port queue and write it to a file */
FT_STATUS uart_dump(FT_HANDLE ftHandle, FILE *fp, long timeout);

/* Write a buffer to an FTDI serial port */
FT_STATUS uart_write(FT_HANDLE ftHandle, unsigned char *buf,
                     unsigned int bytesToWrite);

#endif /* _UART_H */

