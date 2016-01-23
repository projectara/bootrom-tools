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
 * @brief: This file contains the code for "imsgen_test1"
 *
 * This signs a message with RSA, ECC primary and ECC secondary signatures,
 * saving the signatures and the EP_UID in separate files, to be verified by
 * "imsgen_test2"
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
#include <openssl/evp.h>    /* for EVP_MAX_MD_SIZE */
#include "util.h"
#include "parse_support.h"
#include "mcl_arch.h"
#include "mcl_oct.h"
#include "mcl_ecdh.h"
#include "mcl_rand.h"
#include "mcl_rsa.h"
#include "crypto.h"
#include "ims_common.h"
#include "ims_test1.h"


/* Program return values */
#define PROGRAM_SUCCESS     0
#define PROGRAM_WARNINGS    1
#define PROGRAM_ERROR       2


/* Argument handling function prototype */
bool handle_ims_value(const int option, const char * optarg,
                      struct optionx * optx);


/* Parsing args */
static int      sample_compatibility_mode = 0;
static int      num_ims;
static char *   ims_value;
static char *   ims_filename;
static int      ims_index;
static char *   message_filename;

static char *   sample_compatibility_mode_names[] = { "compatibility", NULL };
static char *   ims_value_names[] = { "ims", NULL };
static char *   ims_filename_names[] = { "ims-file", NULL };
static char *   ims_index_names[] = { "ims-index", NULL };
static char *   message_filename_names[] = { "message", "in", NULL };


/* Parsing table */
static struct optionx parse_table[] = {
        { 'I', ims_value_names, NULL,
          &ims_value, 0, OPTIONAL, &store_str, false,
          "The IMS value (binascii, MSb-LSb)" },
        { 'i', ims_filename_names, NULL,
          &ims_filename, 0, OPTIONAL, &store_str, false,
          "The name of the IMS input file" },
        { 'x', ims_index_names, NULL,
          &ims_index, 0, OPTIONAL, &store_hex, false,
          "Which IMS value to choose from --ims-file (zero-based)" },
        { 'm', message_filename_names, NULL,
          &message_filename, 0, REQUIRED, &store_str, false,
          "The name of the message file to sign" },
       { 'c', sample_compatibility_mode_names, NULL,
          &sample_compatibility_mode, 0, STORE_TRUE, NULL, false,
          "100-IMS sample backward compatibility" },
        { 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL }
};

static char all_args[] = "I:i:x:m:c";


/**
 * @brief Post-process and validate the command line args
 *
 * @param argc The number of elements in argv or parsed_argv (std. unix argc)
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
static int postprocess_args(int argc) {
    int status = PROGRAM_SUCCESS;

    if (optind < argc) {
        fprintf(stderr, "ERROR: dangling arguments\n");
        status = PROGRAM_ERROR;
    }

    if ((ims_filename && ims_value) ||
        (!ims_filename && !ims_value)) {
        fprintf(stderr,
                "ERROR: You must specify one of --ims or --ims-file\n");
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
    int status = 0;
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
        if (ims_init("/dev/random", NULL, NULL) != 0) {
            fprintf(stderr, "ERROR: IMS initialization failed\n");
            program_status = PROGRAM_ERROR;
        } else {
            /* Test N IMS values */
            status = test_ims_signing(message_filename,
                                      ims_value,
                                      ims_filename,
                                      ims_index,
                                      sample_compatibility_mode);
            if (status != 0) {
                fprintf(stderr, "ERROR: Unable to sign with this IMS (err %d)\n", status);
                program_status = PROGRAM_ERROR;
            }

            /* Close the DB, IMS file */
            ims_deinit();
        }
    }

    return program_status;
}
