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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include "settings.h"
#include "gpio.h"
#include "jlink_script.h"
#include "uart.h"
#include "reset.h"


/* Parsing table */
static struct option long_options[] = {
    { "test_folder",    required_argument, NULL, 'T' },
    { "bridge_bin",     required_argument, NULL, 'b' },
    { "bridge_ffff",    required_argument, NULL, 'f' },
    { "efuse",          required_argument, NULL, 'e' },
    { "server_bin",     required_argument, NULL, 'B' },
    { "server_ffff",    required_argument, NULL, 'F' },
    { "log",            required_argument, NULL, 'l' },
    { "timeout",        required_argument, NULL, 't' },
    {NULL, 0, NULL, 0}
};


/**
 * @brief Parse a decimal number.
 *
 * @param input The (hopefully) numeric string to parse (argv[i])
 * @param optname The name of the argument, used for error messages.
 * @param num Points to the variable in which to store the parsed number.
 *
 * @returns Returns true on success, false on failure
 */
bool get_num(char * input, const char * optname, uint32_t * num) {
    char *tail = NULL;
    bool success = true;

    *num = strtoul(input, &tail, 10);
    if ((errno != errno) || ((tail != NULL) && (*tail != '\0'))) {
        fprintf (stderr, "Error: invalid %s '%s'\n", optname, optarg);
        success = false;
    }

    return success;
}


/** Run one complete test on the HAPS board
 *
 * Assert the reset on the daughterboard and perform the "in-reset" steps
 * found in "jlink-start" script.
 */

int main(int argc, char * argv[]) {
    char cmd[2048];
    char path[1024];
    FT_HANDLE ftHandleGpioServer = NULL;
    FT_HANDLE ftHandleGpioBridge = NULL;
    FT_HANDLE ftHandleUartBridge = NULL;
    FT_STATUS ftStatus;
    int option;
    int option_index;
    bool success = true;
    bool run_server = false;
    int status = 0;
    char *test_folder = NULL;
    char *bridge_bin = NULL;
    char *bridge_ffff = NULL;
    char *bridge_efuse = NULL;
    char *server_bin = NULL;
    char *server_ffff = NULL;
    char *log_file = NULL;
    uint32_t timeout = 10;  /* 10 seconds */

    /* Parse the command-line arguments */
    while (success) {
        option_index = 0;
        option = getopt_long (argc,
                              argv,
                              "T:b:f:e:B:F:l:t",
                              long_options,
                              &option_index);
        if (option == -1) {
            /* end of options */
            break;
        }
        switch (option) {
        case 'T':   // test folder
            test_folder = optarg;
            break;

        case 'b':   // bridge bootloader bin
            bridge_bin = optarg;
            break;

        case 'f':   // bridge FFFF romimage
            bridge_ffff = optarg;
            break;

        case 'e':   // bridge e-Fuse file
            bridge_efuse = optarg;
            break;

        case 'B':   // server bootloader bin
            server_bin = optarg;
            run_server = true;
            break;

        case 'F':   // server FFFF romimage
            server_ffff = optarg;
            break;

        case 'l':   // log file
            log_file = optarg;
            break;

        case 't':   // timeout
            success = get_num(optarg,
                              "timeout",
                              &timeout);
            break;

        default:
            /* Should never get here */
            printf("?? getopt returned character code 0%o ??\n", option);
            /* and fall through to... */
        case '?':   // extraneous parameter
            fprintf(stderr, "Usage: %s -b=bridge_bin [-f=bridge_ffff] [-e=bridge_efuse]\n", argv[0]);
            fprintf(stderr, "    [-B=server_bin] [-F=server_ffff] [-l=log_file] [-t=timeout]\n");
            break;
        }
    } /* Parsing loop */

    /* Check for required parameters */
    if ((test_folder == NULL) || (bridge_bin == NULL)) {
        printf("Missing required parameters\n");
        return -1;
    }
    printf("Proccessed args:\n"); /*****/
    printf("  test_folder  '%s'\n", test_folder); /*****/
    printf("  bridge_bin   '%s'\n", bridge_bin); /*****/
    printf("  bridge_ffff  '%s'\n", bridge_ffff); /*****/
    printf("  bridge_efuse '%s'\n", bridge_efuse); /*****/
    printf("  server_bin   '%s'\n", bridge_bin); /*****/
    printf("  server_ffff  '%s'\n", bridge_ffff); /*****/
    printf("  log_file     '%s'\n", log_file); /*****/
    printf("  timeout       %u sec.\n", timeout); /*****/

    /* (See: jlink_script.c) */
    if (jlink_prepare_test(test_folder, bridge_efuse, bridge_bin,
                           server_bin)) {
        return -1;
    }

    FILE *fpLog = fopen(log_file, "w");
    if (fpLog == NULL) {
        printf("Failed to create log file: %s (err %d)\n", log_file, errno);
        return -1;
    }
   
    printf("haps_semi: open bridge dbgser\n"); /*****/
    ftStatus = mpsse_init(BRIDGE_DBGSER_ID, &ftHandleUartBridge);
    if (ftStatus != FT_OK) {
        fprintf(stderr, "Can't open bridge debug serial (ftStatus %d)\n",
                ftStatus);
        status = -1;
        goto ErrorReturn;
    }
    FT_SetFlowControl(ftHandleUartBridge, FT_FLOW_NONE, 0x00, 0x00);
    FT_SetDataCharacteristics(ftHandleUartBridge, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
    FT_SetBaudRate(ftHandleUartBridge, FT_BAUD_115200);

    printf("haps_semi: open server gpio\n"); /*****/
    ftStatus = mpsse_init(SERVER_GPIO_ID, &ftHandleGpioServer);
    printf("haps_semi: open bridge gpio\n"); /*****/
    ftStatus |= mpsse_init(BRIDGE_GPIO_ID, &ftHandleGpioBridge);
    if (ftStatus != FT_OK) {
        fprintf(stderr, "Can't open GPIO devices (ftStatus %d)\n", ftStatus);
        status = -1;
        goto ErrorReturn;
    }

    /* Assert the reset on bridge and server */
    reset_gpio_assert(ftHandleGpioServer);
    reset_gpio_assert(ftHandleGpioBridge);

    /* Flash the server */
    if (run_server && (server_ffff != NULL)) {
        printf("Flashing the Server...\n");
        sprintf(cmd, "%s/spirom_write A %s", FTDI_DIR, server_ffff);
        status = system(cmd);
    }
    /* Flash the bridge */
    if (bridge_ffff != NULL) {
        printf("Flashing the Bridge...\n");
        sprintf(cmd, "%s/spirom_write B %s", FTDI_DIR, bridge_ffff);
        status = system(cmd);
    }

    /* Reset the HAPS board to clear any (un)trusted status */
    status = reset_haps_pulse();
    sleep(2);


    /* Shell out to run the in-reset J-link script */
    printf("Reset-phase J-link scripts (bridge, server)\n"); /*****/
    sprintf(cmd,
            "JLinkExe -SelectEmuBySN %s -CommanderScript %s",
            BRIDGE_JLINK_SN, jlink_start_script);
    printf ("system(%s)...\n", cmd); /*****/
    status = system(cmd);
    printf ("system returned %d\n", status); /*****/
    if (run_server) {
        sprintf(cmd,
                "JLinkExe -SelectEmuBySN %s -CommanderScript %s",
                SERVER_JLINK_SN, jlink_start_script);
        printf ("system(%s)...\n", cmd); /*****/
        status = system(cmd);
        printf ("system returned %d\n", status); /*****/
    }

    /*
     * De-assert the reset on bridge and optionally server, and shell out
     * to run the post-reset Jlink script(s)
     */
    if (run_server) {
        reset_gpio_deassert(ftHandleGpioServer);
        sprintf(cmd, "JLinkExe -SelectEmuBySN %s -CommanderScript %s",
                SERVER_JLINK_SN, server_jlink_script);
        printf ("system(%s)...\n", cmd); /*****/
        status = system(cmd);
        printf ("system returned %d\n", status); /*****/
    }
    reset_gpio_deassert(ftHandleGpioBridge);
    sprintf(cmd, "JLinkExe -SelectEmuBySN %s -CommanderScript %s",
            BRIDGE_JLINK_SN, bridge_jlink_script);
    printf ("system(%s)...\n", cmd); /*****/
    status = system(cmd);
    printf ("system returned %d\n", status); /*****/

    /* Dump the captured debug output, to <log_file> */
    /* (See: uart.c) */
    uart_dump(ftHandleUartBridge, fpLog, timeout);

ErrorReturn:
    jlink_cleanup_test();
    if (fpLog != NULL) {
        fclose(fpLog);
    }
    if (ftHandleUartBridge != NULL) {
        FT_Close(ftHandleUartBridge);
    }
    if (ftHandleGpioServer != NULL) {
        FT_Close(ftHandleGpioServer);
    }
    if (ftHandleGpioBridge != NULL) {
        FT_Close(ftHandleGpioBridge);
    }

    printf("\n%s returns status %d\n", argv[0], status); /*****/
    return status;
}
    

