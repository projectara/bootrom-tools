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
 * @brief: This file contains the code for "display-ffff" a Linux command-line
 * app used to display a Flash Format For Firmware(FFFF) file, used by the
 * bootloader.
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
#include "ffff.h"
#include "ffff_common.h"
#include "ffff_in.h"
#include "ffff_out.h"
#include "ffff_map.h"
#include "ffff_print.h"



/* Program return values */
#define PROGRAM_SUCCESS     0
#define PROGRAM_WARNINGS    1
#define PROGRAM_ERRORS      2


/* FFFF parsing args */
int map_flag = false;

static char * map_flag_names[] = { "map", NULL };

static const char map_flag_help[] =
        "Create a map file of the FFFF headers and each FFFF sections";


/* Parsing table */
static struct optionx parse_table[] = {
    { 'm', map_flag_names, NULL,  &map_flag, 0,
      STORE_TRUE, &store_flag, 0, map_flag_help },
    { 0, NULL, NULL, NULL, 0, 0, NULL, 0 , NULL}
};


/* The 1-char tags for all args */
static char * all_args = "m";


static char * epilog =
    "NOTE: elements are specified as [<element_type> <file>  <element_option>]...\n"
    "   <element_type> ::= [--s2f | --s3f | --ims | --cms | --data]\n"
    "   <element_option> ::= {--eclass} {--eid} {--eloc} {--elen}";


/**
 * @brief Entry point for the display-FFFF application
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
    parse_tbl = new_argparse(parse_table, argv[0], NULL, epilog, NULL,
                             NULL);
    if (parse_tbl) {
        success =  parse_args(argc, argv, all_args, parse_tbl);
        if (!success) {
            program_status = parser_help? PROGRAM_SUCCESS : PROGRAM_ERRORS;
        }
    } else {
        success = false;
    }

    if (success) {
        /* Process any remaining command line arguments as filenames */
        if (optind < argc) {
            for (; optind < argc; optind++) {
                struct ffff * romimage = NULL;

                /* Read in the TFTF file as a blob... */
                romimage = read_ffff_romimage(argv[success]);

                if (romimage) {
                    /* ...and print it out */
                    print_ffff_file(romimage, argv[optind]);

                    if (map_flag) {
                        /* ...and write out the map file */
                       success = write_ffff_map_file(romimage, argv[optind]);
                    }
                    romimage = free_ffff(romimage);
                } else {
                    fprintf(stderr, "ERROR: file given was not FFFF\n");
                    program_status = PROGRAM_ERRORS;
                }
            }
            putchar ('\n');
        } else {
            fprintf(stderr, "ERROR: No TFTF files to display\n");
            program_status = PROGRAM_ERRORS;
        }
    }

    parse_tbl = free_argparse(parse_tbl);
    return program_status;
}

