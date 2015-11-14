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
#include "tftf.h"
#include "tftf_common.h"
#include "tftf_in.h"
#include "tftf_out.h"
#include "tftf_map.h"
#include "tftf_print.h"
#include "sign.h"


/* Program return values */
#define PROGRAM_SUCCESS     0
#define PROGRAM_WARNINGS    1
#define PROGRAM_ERRORS      2


parse_entry passin_modes[] = {
    {"prompt", PASSIN_PROMPT},
    {"pass:",  PASSIN_PASSIN},
    {"stdin",  PASSIN_STDIN},
    {NULL, 0}
};

parse_entry signature_algorithms[] = {
    {"rsa2048-sha256", ALGORITHM_TYPE_RSA2048_SHA256},
    {NULL, 0}
};

parse_entry package_types[] = {
        {"s2fsk", KEY_TYPE_S2FSK},
        {"s3fsk", KEY_TYPE_S3FSK},
        {NULL, 0}
};

parse_entry signature_formats[] = {
        {"standard", FORMAT_TYPE_STANDARD},
        {"es3",      FORMAT_TYPE_ES3},
        {NULL, 0}
};


/* TFTF parsing args */
static int      verbose_flag = false;
static int      retry_flag = false;
static int      check_flag = false;
static char *   key_filename;
static char *   suffix;
static uint32_t package_type;
static uint32_t signature_algorithm;
static uint32_t signature_format;
static uint32_t passin_mode;
static char     passphrase_buffer[256];
char *          passphrase;


/* TFTF parsing callbacks */
bool handle_passin(const int option, const char * optarg,
                   struct optionx * optx);
bool handle_package_type(const int option, const char * optarg,
                         struct optionx * optx);
bool handle_algorithm(const int option, const char * optarg,
                      struct optionx * optx);
bool handle_format(const int option, const char * optarg,
                   struct optionx * optx);

/* Parsing table */
static struct optionx parse_table[] = {
    { 'p', "passin",                &passin_mode,           PASSIN_PROMPT,  DEFAULT_VAL,    &handle_passin, 0 },
    { 't', "type",                  &package_type,          0,              REQUIRED,       &handle_package_type, 0 },
    { 's', "suffix",                &suffix,                0,              0,              &store_str, 0 },
    { 'a', "signature-algorithm",   &signature_algorithm,   0,              REQUIRED,       &handle_algorithm, 0 },
    { 'f', "format",                &signature_format,      0,              REQUIRED,       &handle_format, 0 },
    { 'k', "key",                   &key_filename,          0,              REQUIRED,       &store_str, 0 },
    { 'r', "retry",                 &retry_flag,            0,              DEFAULT_VAL | STORE_TRUE,     NULL, false },
    { 'c', "check",                 &check_flag,            0,              DEFAULT_VAL | STORE_TRUE,     NULL, false },
    { 'v', "verbose",               &verbose_flag,          0,              DEFAULT_VAL | STORE_TRUE,     NULL, false },
    { 0, NULL, NULL, 0, 0, NULL, 0 }
};

static char all_args[] = "p:t:s:a:f:k:";

static char * usage_strings[] =
{
    "Usage: sign-tftf --type [s2fsk | s3fsk] --key pemfile \\",
    "                 --signature-algorithm [rsa2048-sha256] \\"
    "                 --format [standard | es3]\\"
    "                 --passin [pass:<passphrase> | stdin | prompt]\\",
    "                 {--suffix <string>} {--verbose} <file>...",
    "Where:",
    "    -v, --verbose",
    "        Display the TFTF header and a synopsis of each TFTF section",
    "    <file> is a tftf file",
};

/**
 * @brief Callback to validate and store the signing key passphrase.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_passin(const int option, const char * optarg,
                     struct optionx * optx) {
    *(uint32_t*)optx->var_ptr = kw_to_token(optarg, passin_modes);
    if (*(uint32_t*)optx->var_ptr == TOKEN_NOT_FOUND) {
        /* Check if it is "pass:<string>" */
        if (strncmp(optarg, "pass:", 5) != 0) {
            fprintf(stderr, "ERROR: Invalid --%s: %s\n", optx->name, optarg);
            return false;
        } else {
            optarg += 5;    /* step over the "pass:" */
            *(uint32_t*)optx->var_ptr = PASSIN_PASSIN;
            passphrase = malloc(strlen(optarg) + 1);
            if (!passphrase) {
                fprintf(stderr, "ERROR: Can't allocate passphrae\n");
                return false;
            } else {
                strcpy(passphrase, optarg);
            }
        }
    }
    return true;
}


/**
 * @brief Callback to validate and store the TFTF key type.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_package_type(const int option, const char * optarg,
                     struct optionx * optx) {
    *(uint32_t*)optx->var_ptr = kw_to_token(optarg, package_types);
    if (*(uint32_t*)optx->var_ptr == TOKEN_NOT_FOUND) {
        fprintf(stderr, "ERROR: Invalid --%s: %s\n", optx->name, optarg);
        return false;
    }
    return true;
}


/**
 * @brief Callback to validate and store the TFTF signature algorithm.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_algorithm(const int option, const char * optarg,
                      struct optionx * optx) {
    *(uint32_t*)optx->var_ptr = kw_to_token(optarg, signature_algorithms);
    if (*(uint32_t*)optx->var_ptr == TOKEN_NOT_FOUND) {
        fprintf(stderr, "ERROR: Invalid --%s: %s\n", optx->name, optarg);
        return false;
    }
    return true;
}


/**
 * @brief Callback to validate and store the TFTF signature format.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_format(const int option, const char * optarg,
                   struct optionx * optx) {
    *(uint32_t*)optx->var_ptr = kw_to_token(optarg, signature_formats);
    if (*(uint32_t*)optx->var_ptr == TOKEN_NOT_FOUND) {
        fprintf(stderr, "ERROR: Invalid --%s: %s\n", optx->name , optarg);
        return false;
    }
    return true;
}


/**
 * @brief Print out the usage message
 *
 * @param None
 *
 * @returns Nothing
 */
void usage(void) {
    int i;

    for (i = 0; i < _countof(usage_strings); i++) {
        fprintf(stderr, "%s\n", usage_strings[i]);
    }
}


/**
 * @brief Get the passphrase from the user
 *
 * @param key_filename The name of the private key file.
 *        If key_filename is NULL, we simply read the next line from stdin.
 *        If key_filename is non-NULL, we switch stdin to no-echo, prompt the
 *        user for the passphrase, and switch the mode back.
 *
 * @returns A pointer to the passphrase on success, NULL on failure.
 */
char * get_passphrase(uint32_t passin_mode) {
    char * pass = NULL;
    size_t last;


    switch (passin_mode) {
    case PASSIN_PROMPT:
        pass = getpass("Enter PEM pass phrase: ");
        break;

    case PASSIN_STDIN:
        /* Get the line from stdin and strip off the newline */
        pass = fgets(passphrase_buffer, sizeof(passphrase_buffer), stdin);
        if (pass) {
            /* strip the newline */
            last = strlen(passphrase_buffer) - 1;
            if (passphrase_buffer[last] == '\n') {
                passphrase_buffer[last] = '\0';
            }
            pass = passphrase_buffer;
        }
        break;

    case PASSIN_PASSIN:
        pass = passphrase;
    }

    return pass;
}


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
    bool passphrase_invalid = false;
    struct argparse * parse_tbl = NULL;
    int program_status = PROGRAM_SUCCESS;

    /* Parse the command line arguments */
    parse_tbl = new_argparse(parse_table, NULL);
    if (parse_tbl) {
        success =  parse_args(argc, argv, all_args, parse_tbl);
        parse_tbl = free_argparse(parse_tbl);
    } else {
        success = false;
    }


    /* Perform any argument validation/post-processing */
    printf("passin_mode %u\n", passin_mode);
    passphrase = get_passphrase(passin_mode);
    if (!passphrase) {
        fprintf(stderr, "ERROR: Missing passphrase for private key %s\n",
                key_filename);
    }


    /* Process the remaining command line arguments as files to sign. */
    if (success) {
        if (optind < argc) {
            if (sign_init(key_filename)) {
                for (; optind < argc; optind++) {
                    if (!sign_tftf(argv[optind], signature_format,
                                   package_type, signature_algorithm,
                                   key_filename, suffix, !check_flag,
                                   verbose_flag, &passphrase_invalid)) {
                        fprintf(stderr, "ERROR: Unable to sign %s\n",
                                argv[optind]);
                        success = false;
                    }
                    if (passphrase_invalid && retry_flag &&
                        (passin_mode == PASSIN_PROMPT)) {
                        fprintf(stderr, "ERROR: Invalid passphrase\n");
                        success = false;
                        break;
                    }
                }
                sign_deinit();
            } else {
                fprintf(stderr, "ERROR: Couldn't initialize crypto\n");
                success = false;
            }
        } else {
            fprintf(stderr, "ERROR: No TFTF files to sign\n");
            success = false;
        }
    }

    if (!success) {
        program_status = PROGRAM_ERRORS;
    }

    return program_status;
}
