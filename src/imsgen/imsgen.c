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
 * @brief: This file contains the code for "sign-tftf" a Linux command-line
 * app used to sign one or more Trusted Firmware Transfer Format (TFTF) files
 * used by the secure bootloader.
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
#include <libgen.h>
#include "util.h"
#include "parse_support.h"
#include "crypto.h"
#include "ims.h"


/* Program return values */
#define PROGRAM_SUCCESS     0
#define PROGRAM_WARNINGS    1
#define PROGRAM_ERROR       2


/* Parsing args */
static int      num_ims;
static char *   database_name;
static char *   ims_filename;
static char *   prng_seed_filename;
static char *   prng_seed_string;

static char *   num_ims_names[] = { "num", "num-ims", NULL };
static char *   database_name_names[] = { "db", "database", NULL };
static char *   ims_filename_names[] = { "out", "ims", NULL };
static char *   prng_seed_filename_names[] = { "seed-file", NULL };
static char *   prng_seed_string_names[] = { "seed", NULL };


/* Parsing table */
static struct optionx parse_table[] = {
    { 's', prng_seed_filename_names, NULL,
      &prng_seed_filename, 0, OPTIONAL, &store_str, false,
      "The file containing the PRNG seed string" },
    { 'f', prng_seed_string_names, NULL,
      &prng_seed_string, 0, OPTIONAL, &store_str, false,
      "The PRNG seed string (hex digits)" },
    { 'o', ims_filename_names, NULL,
      &ims_filename, 0, REQUIRED, &store_str, false,
      "The name of the IMS output file" },
    { 'd', database_name_names, NULL,
      &database_name, 0, REQUIRED, &store_str, false,
      "The name of the certificate database" },
    { 'n', num_ims_names, NULL,
      &num_ims, 0, REQUIRED, &store_hex, false,
      "The number of IMS values to generate" },
    { 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL }
};

static char all_args[] = "s:o:d:n:";


/**
 * @brief Post-process and validate the command line args
 *
 * @param argc The number of elements in argv or parsed_argv (std. unix argc)
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
int postprocess_args(int argc) {
    int status = PROGRAM_SUCCESS;

    if (optind < argc) {
        fprintf(stderr, "ERROR: dangling arguments\n");
        status = PROGRAM_ERROR;
    }

    if (num_ims < 1) {
        fprintf(stderr, "ERROR: --num must be >= 1\n");
        status = PROGRAM_ERROR;
    }

    if ((prng_seed_filename && prng_seed_string) ||
        (!prng_seed_filename && !prng_seed_string)) {
        fprintf(stderr, "ERROR: You must specify one of --seed or --seed-file\n");
        status = PROGRAM_ERROR;
    }

    return status;
}

/**
 * @brief Entry point for the imsgen application
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
    uint32_t count;

    /* Parse the command line arguments */
    parse_tbl = new_argparse(parse_table, argv[0], NULL, NULL, "<file>...", NULL);
    if (parse_tbl) {
        if (!parse_args(argc, argv, all_args, parse_tbl)) {
            program_status = parser_help? PROGRAM_SUCCESS : PROGRAM_ERROR;
        }
        parse_tbl = free_argparse(parse_tbl);

        /* Perform any argument validation/post-processing */
        if (program_status == PROGRAM_SUCCESS) {
            program_status = postprocess_args(argc);
        }
    } else {
        program_status = PROGRAM_ERROR;
    }


    if (program_status == PROGRAM_SUCCESS) {
        /* Open the DB, IMS file, etc.  */
        if (ims_init(prng_seed_filename, prng_seed_string, ims_filename, database_name) != 0) {
            fprintf(stderr, "ERROR: IMS generation initialization failed\n");
            program_status = PROGRAM_ERROR;
        } else {
            /* Generate N IMS values */
            for (count = 0; count < num_ims; count++) {
                /*****/printf("imsgen: IMS %d/%d\n", count + 1, num_ims);
                if (ims_generate() != 0) {
                    fprintf(stderr,
                            "ERROR: created only %u of %u IMS values\n",
                            count, num_ims);
                    program_status = PROGRAM_ERROR;
                    break;
                }
            }

            /* Close the DB, IMS file */
            ims_deinit();
        }
    }

    /*****/printf("imsgen: done\n");
    return program_status;
}
