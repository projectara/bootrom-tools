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

/**
 *
 * @brief: This file contains the code for "display-tftf" a Linux command-line
 * app used to display an Trusted Firmware Transfer Format (TFTF) file used
 * by the secure bootloader.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include "util.h"
#include "parse_support.h"
#include "tftf.h"
#include "tftf_common.h"
#include "tftf_in.h"
#include "tftf_out.h"
#include "tftf_map.h"
#include "tftf_print.h"


/* Program return values */
#define PROGRAM_SUCCESS     0
#define PROGRAM_WARNINGS    1
#define PROGRAM_ERRORS      2


/* TFTF parsing args */
int verbose_flag = false;
int map_flag = false;

/* Parsing table */
static struct optionx parse_table[] = {
    { 'v', "verbose", NULL, &verbose_flag, 0,
      DEFAULT_VAL | STORE_TRUE, NULL, 0,
      "Display the TFTF header and a synopsis of each TFTF section"},
    { 'm', "map", NULL, &map_flag, 0,
      DEFAULT_VAL | STORE_TRUE, NULL, 0,
      "Create a map file of the TFTF header and each TFTF section"},
    { 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL }
};


/**
 * @brief Entry point for the display-tftf application
 *
 * @param argc The number of elements in argv or parsed_argv (std. unix argc)
 * @param argv The unix argument vector - an array of pointers to strings.
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
int main(int argc, char * argv[]) {
    bool success = true;
    struct argparse * parse_tbl = NULL;
    int program_status = PROGRAM_SUCCESS;

    /* Parse the command line arguments */
    parse_tbl = new_argparse(parse_table, argv[0], NULL,
                             "<file> is a tftf file",
                             "<file>...", NULL);
    if (parse_tbl) {
        success =  parse_args(argc, argv, "", parse_tbl);
        parse_tbl = free_argparse(parse_tbl);
    } else {
        success = false;
    }


    if (success) {
        /* Print any remaining command line arguments (not options). */
        if (optind < argc) {
            for (; optind < argc; optind++) {
                tftf_header * tftf_hdr = NULL;
                ssize_t tftf_size;

                /* Read in the TFTF file as a blob... */
                tftf_hdr = (tftf_header *)alloc_load_file(argv[optind],
                                                          &tftf_size);
                if (tftf_hdr) {
                    /* ...and print it out */
                    print_tftf_file(tftf_hdr, argv[optind]);

                    if (map_flag) {
                        /* ...and write out the map file */
                        success = write_tftf_map_file(tftf_hdr, argv[optind]);
                    }
                }
            }
            putchar ('\n');
        } else {
            fprintf(stderr, "ERROR: No TFTF files to display\n");
            program_status = PROGRAM_ERRORS;
        }
    }


    return program_status;
}

