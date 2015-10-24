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

#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include "jlink_script.h"



/**
 * @brief Issue the e-Fuse setup string to the script file.
 *
 * @param fp The open File object for the script file
 *
 * @returns Returns 0 on success, -1 on error
 */
static int prepare_efuse(FILE *fp, char * efuse) {
    FILE *fe = NULL;
    unsigned int buf[20];

    fe = fopen(efuse, "r");
    if (fe == NULL) {
        fprintf(stderr, "Couldn't open e-Fuse file %s (err %d)\n",
                efuse, errno);
        return -1;
    }
    fscanf(fe, "SN[63:0] = %x_%x\n", &buf[1], &buf[0]);
    printf("SN[63:0] = %08x_%08x\n", buf[1], buf[0]);
    fprintf(fp, "w4 0x40084300 0x%08X\n", buf[0]);
    fprintf(fp, "w4 0x40084304 0x%08X\n", buf[1]);

    fscanf(fe, "PID[31:0] = %x\n", &buf[0]);
    printf("PID[31:0] = %08x\n", buf[0]);
    fprintf(fp, "w4 0x40000704 0x%08X\n", buf[0]);

    fscanf(fe, "VID[31:0] = %x\n", &buf[0]);
    printf("VID[31:0] = %08x\n", buf[0]);
    fprintf(fp, "w4 0x40000700 0x%08X\n", buf[0]);

    fscanf(fe, "CMS[215:0] = %x_%x_%x_%x_%x_%x_%x\n",
           &buf[6], &buf[5], &buf[4], &buf[3], &buf[2], &buf[1], &buf[0]);
    printf("CMS[215:0] = %x_%08x_%08x_%08x_%08x_%08x_%08x\n",
           buf[6], buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);
    fprintf(fp, "w4 0x40084200 0x%08X\n", buf[0]);
    fprintf(fp, "w4 0x40084204 0x%08X\n", buf[1]);
    fprintf(fp, "w4 0x40084208 0x%08X\n", buf[2]);
    fprintf(fp, "w4 0x4008420C 0x%08X\n", buf[3]);
    fprintf(fp, "w4 0x40084210 0x%08X\n", buf[4]);
    fprintf(fp, "w4 0x40084214 0x%08X\n", buf[5]);
    fprintf(fp, "w4 0x40084218 0x%08X\n", buf[6]);

    fscanf(fe, "SCR[7:0] = %x\n", &buf[0]);
    printf("SCR[7:0] = %x\n", buf[0]);
    //fprintf(fp, "w4 0x40084308 0x%08X\n", buf[0]);
    
    fscanf(fe, "IMS[279:0] = %x_%x_%x_%x_%x_%x_%x_%x_%x\n",
           &buf[8], &buf[7], &buf[6], &buf[5], &buf[4],
           &buf[3], &buf[2], &buf[1], &buf[0]);
    printf("IMS[279:0] = %X_%08x_%08x_%08x_%08x_%08x_%08x_%08x_%08x\n",
           buf[8], buf[7], buf[6], buf[5], buf[4],
           buf[3], buf[2], buf[1], buf[0]);
    fprintf(fp, "w4 0x40084100 0x%08X\n", buf[0]);
    fprintf(fp, "w4 0x40084104 0x%08X\n", buf[1]);
    fprintf(fp, "w4 0x40084108 0x%08X\n", buf[2]);
    fprintf(fp, "w4 0x4008410C 0x%08X\n", buf[3]);
    fprintf(fp, "w4 0x40084110 0x%08X\n", buf[4]);
    fprintf(fp, "w4 0x40084114 0x%08X\n", buf[5]);
    fprintf(fp, "w4 0x40084118 0x%08X\n", buf[6]);
    fprintf(fp, "w4 0x4008411C 0x%08X\n", buf[7]);
    fprintf(fp, "w4 0x40084120 0x%08X\n", buf[8]);

    fclose(fe);
    return 0;
}


/**
 * @brief Create the post-reset phase J-link command files.
 *
 * Creates the bridge script and optionally the server script
 *
 * @param test_folder The path to where to create the J-link scripts
 * @param efuse The .efz file to parse and add to the script
 * @param bridge_bin The bridge bootrom.bin file
 * @param server_bin The server bootrom.bin file
 *
 * @returns Returns 0 on success, -1 on failure
 */
int jlink_prepare_test(char *test_folder, char *efuse,
                       char *bridge_bin, char *server_bin) {
    FILE *fp;

    sprintf(jlink_start_script, "%s/jlink_start_script", test_folder);
    sprintf(server_jlink_script, "%s/jlink_script_server", test_folder);
    sprintf(bridge_jlink_script, "%s/jlink_script_bridge", test_folder);


    /* Create the in-reset script, common to server and bridge */
    fp = fopen(jlink_start_script, "w");
    if (fp == NULL) {
        printf("Can't create J-link start script %s (err %d)\n",
               jlink_start_script, errno);
        return -1;
    } else {
        fprintf(fp, "w4 0xE000EDFC 0x01000001\n");
        fprintf(fp, "w4 0x40000100 0x1\n");
        fprintf(fp, "q\n");
        fclose(fp);
    }


    /* Create the post-reset script to the bridge */
    fp = fopen(bridge_jlink_script, "w");
    if (fp == NULL) {
        printf("Can't create bridge script %s (err %d)\n",
               bridge_jlink_script, errno);
        return -1;
    }

    fprintf(fp, "halt\n");
    fprintf(fp, "loadbin %s 0x00000000\n", bridge_bin);
    fprintf(fp, "w4 0xE000EDFC 0x01000000\n");
    
    if (prepare_efuse(fp, efuse) != 0) {
        fclose(fp);
        return -1;
    }

    fprintf(fp, "w4 0x40000000 0x1\n");
    fprintf(fp, "w4 0x40000100 0x1\n");
    fprintf(fp, "q\n");
    fclose(fp);

    /* Optionally create the post-reset script to the server */
    if (server_bin != NULL) {
        fp = fopen(server_jlink_script, "w");
        if (fp == NULL) {
            printf("Can't create server script %s (err %d)\n",
                   server_jlink_script, errno);
            return -1;
        }

        fprintf(fp, "halt\n");
        fprintf(fp, "loadbin %s 0x00000000\n", server_bin);
        fprintf(fp, "w4 0xE000EDFC 0x01000000\n");

        fprintf(fp, "w4 0x40000000 0x1\n");
        fprintf(fp, "w4 0x40000100 0x1\n");
        fprintf(fp, "q\n");
        fclose(fp);
    }

    return 0;
}


/**
 * @brief Remove a single file, c/w error handling.
 *
 * Creates the bridge script and optionally the server script
 *
 * @param fname The pathanme of the file to snuff out
 *
 * @returns Returns 0 on success, -1 on failure
 */
static int remove_file(char * fname) {
    int status = 0;

    if ((*fname != '\0') && (access( fname, F_OK ) != -1)) {
        status = unlink(fname);
        if (status != 0) {
            fprintf(stderr, "Can't remove %s (err %d)\n", fname, errno);
        } else {
            /* Note that file is gone and the fname is no longer usable */
            *fname = '\0';
        }
    }
    return status;
}


/**
 * @brief Remove the J-link command files.
 *
 * Creates the bridge script and optionally the server script
 *
 * @param test_folder The path to where to create the J-link scripts
 * @param efuse The .efz file to parse and add to the script
 * @param bridge_bin The bridge bootrom.bin file
 * @param server_bin The server bootrom.bin file
 *
 * @returns Returns 0 on success, -1 on failure
 */
int jlink_cleanup_test(void) {
    int status = 0;

    status = remove_file(jlink_start_script);
    status |= remove_file(bridge_jlink_script);
    status |= remove_file(server_jlink_script);

    return status;
}

