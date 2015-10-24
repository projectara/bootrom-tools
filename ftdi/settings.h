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

/*
 * HAPS test rig settings
 */

/* Daughterboard A = Server, Daughterboard B = Bridge */
#define MONITOR_NAME       "FT232R USB UART"  /* HAPS ChipIt monitor UART */
#define SERVER_GPIO_NAME   "Adafruit A"   /* Server Adafruit GPIO adapter */
#define BRIDGE_GPIO_NAME   "Adafruit B"   /* BridgeAdafruit GPIO adapter */
#define SERVER_SPIROM_NAME "USB <-> Serial Converter A A"  /* Daughterboard A SPIROM */
#define SERVER_DBGSER_NAME "USB <-> Serial Converter A C"  /* Daughterboard A DbgSer UART */
#define BRIDGE_SPIROM_NAME "USB <-> Serial Converter B A"  /* Daughterboard B SPIROM */
#define BRIDGE_DBGSER_NAME "USB <-> Serial Converter B C"  /* Daughterboard B DbgSer UART */



/* TODO: pull these settings in from environment variables or scanning FTDI devices */
#ifdef JUN
    /* Jun's settings */
    #define BRIDGE_JLINK_SN  "504301982"
    #define SERVER_JLINK_SN  "504301989"
    #define MONITOR_SN       "A500DAM9"   /* HAPS ChipIT monitor UART */
    #define SERVER_GPIO_SN   "FTZ0SGEY"   /* Daughterboard A Adafruit */
    #define BRIDGE_GPIO_SN   "FTZ0SH7M"   /* Daughterboard B Adafruit */
    #define SERVER_SPIROM_SN "FTZ08Q8FA"  /* Daughterboard A SPIROM */
    #define SERVER_DBGSER_SN "FTZ08Q8FC"  /* Daughterboard A DbgSer UART */
    #define BRIDGE_SPIROM_SN "FTZ08QANA"  /* Daughterboard B SPIROM */
    #define BRIDGE_DBGSER_SN "FTZ08QANC"  /* Daughterboard B DbgSer UART */
    #define FTDI_DIR         "/home/junl/work/ftdi"
#else
    /* Morgan's settings */
    #define BRIDGE_JLINK_SN  "504302259"
    #define SERVER_JLINK_SN  "504302001"
    #define MONITOR_SN       "A500DAKQ"   /* HAPS ChipIT monitor UART */   
    #define SERVER_GPIO_SN   "FTZ0Q9FM"   /* Daughterboard A Adafruit */
    #define BRIDGE_GPIO_SN   "FTZ0SHAE"   /* Daughterboard B Adafruit */
    #define SERVER_SPIROM_SN "FTZ0Q6DXA"  /* Daughterboard A SPIROM */
    #define BRIDGE_SPIROM_SN "FTZ0Q6EWA"  /* Daughterboard B SPIROM */
    #define SERVER_DBGSER_SN "FTZ0Q6DXC"  /* Daughterboard A DbgSer UART */
    #define BRIDGE_DBGSER_SN "FTZ0Q6EWC"  /* Daughterboard B DbgSer UART */
    #define FTDI_DIR         "/home/dev/bootrom-tools/ftdi"
#endif

#define FTDI_USE_DESCRIPTION

#ifdef FTDI_USE_DESCRIPTION
    /* Use the FTDI description string as the identifier */
    #define MONITOR_ID       MONITOR_NAME
    #define SERVER_GPIO_ID   SERVER_GPIO_NAME
    #define BRIDGE_GPIO_ID   BRIDGE_GPIO_NAME
    #define SERVER_SPIROM_ID SERVER_SPIROM_NAME
    #define SERVER_DBGSER_ID SERVER_DBGSER_NAME
    #define BRIDGE_SPIROM_ID BRIDGE_SPIROM_NAME
    #define BRIDGE_DBGSER_ID BRIDGE_DBGSER_NAME
#else
    /* Use the FTDI serial number string as the identifier */
    #define MONITOR_ID       MONITOR_SN
    #define SERVER_GPIO_ID   SERVER_GPIO_SN
    #define BRIDGE_GPIO_ID   BRIDGE_GPIO_SN
    #define SERVER_SPIROM_ID SERVER_SPIROM_SN
    #define SERVER_DBGSER_ID SERVER_DBGSER_SN
    #define BRIDGE_SPIROM_ID BRIDGE_SPIROM_SN
    #define BRIDGE_DBGSER_ID BRIDGE_DBGSER_SN
#endif

#ifndef _SETTINGS_H
#define _SETTINGS_H


/*
 * Function prototypes:
 */

/* Read N bytes from an FTDI serial port into a buffer */
FT_STATUS uart_read(FT_HANDLE ftHandle, unsigned char *buf, unsigned int *bytesRead);

#endif /* _SETTINGS_H */

