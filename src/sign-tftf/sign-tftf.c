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

/* The following constant *SHOULD* be in tftf.h */
#define SIGNATURE_KEY_NAME_LENGTH   96


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


/* TFTF parsing args */
static int      verbose_flag = false;
static int      no_retry_flag = false;
static int      check_flag = false;
static char *   key_filename;
static char *   key_domain = "s2fvk.projectara.com";
static char     key_id_buffer[256];
static char *   key_id;
static uint32_t signature_algorithm;
static uint32_t passin_mode;
static char     passphrase_buffer[256];
char *          passphrase;

static char *   verbose_flag_names[] = { "verbose", NULL };
static char *   no_retry_flag_names[] = { "no-retry", NULL };
static char *   check_flag_names[] = { "check", NULL };
static char *   key_filename_names[] = { "key", NULL };
static char *   key_domain_names[] = { "domain", NULL };
static char *   key_id_names[] = { "id", NULL };
static char *   signature_algorithm_names[] =
    { "algorithm", NULL };
static char *   passin_mode_names[] = { "passin", NULL };


/* TFTF parsing callbacks */
bool handle_passin(const int option, const char * optarg,
                   struct optionx * optx);
bool handle_algorithm(const int option, const char * optarg,
                      struct optionx * optx);

/* Parsing table */
static struct optionx parse_table[] = {
    { 'p', passin_mode_names, "[pass:<passphrase> | stdin | (prompt)]",
      &passin_mode, PASSIN_PROMPT,  DEFAULT_VAL, &handle_passin, 0,
      "Display the signed TFTF header when done"},
    { 'a', signature_algorithm_names, "rsa2048-sha256",
      &signature_algorithm, 0, REQUIRED, &handle_algorithm, 0,
      "The cryptographic signature algorithm used in the PEM file "
      "(typ. rsa2048-sha256)" },
    { 'k', key_filename_names, "<pemfile>",
      &key_filename, 0, REQUIRED, &store_str, 0 ,
      "The name of the signing key PEM file "
      "(e.g. 'test-151210-02-20151212-01.private.pem')"},
    { 'd', key_domain_names, "s2fvk.projectara.com",
      &key_domain, 0, REQUIRED, &store_str, 0 ,
      "The key domain - the right-hand part of the validation key name"},
    { 'i', key_id_names, NULL,
      &key_id, 0, OPTIONAL, &store_str, 0 ,
      "The ID of the key (instead of deriving it from the key filename)"},
    { 'r', no_retry_flag_names, NULL,
      &no_retry_flag, 0, DEFAULT_VAL | STORE_TRUE, NULL, false,
      "If --passin prompt is specified, exit with an error status if\n"
      "the password is invalid." },
    { 'c', check_flag_names, NULL,
      &check_flag, 0, DEFAULT_VAL | STORE_TRUE, NULL, false,
      "Check that the parameters are sound, that the specified TFTF file\n"
      "exists, and that the password is correct, but do not modify the\n"
      "TFTF file. (Optional)" },
    { 'v', verbose_flag_names, NULL,
      &verbose_flag, 0, DEFAULT_VAL | STORE_TRUE, NULL, false,
      "Display the signed TFTF header when done" },
    { 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL }
};

static char all_args[] = "p:a:f:k:d:i:rcv";


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
            fprintf(stderr, "ERROR: Invalid --%s: %s\n",
                    (optx->long_names)[0], optarg);
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
        fprintf(stderr, "ERROR: Invalid --%s: %s\n",
                (optx->long_names)[0], optarg);
        return false;
    }
    return true;
}


/**
 * @brief Extract the key ID from the key pathname
 *
 * @param key_ID The (optional) user-supplied ID string.
 * @param key_filename The name of the private key file.
 *
 * @returns A pointer to the key_id on success, NULL on failure.
 */
char * get_id(char *key_id, const char * key_filename) {
    char * loc_key_filename = NULL;
    char * loc_key_name;

    /**
     * Extract the ID from the key filename if the user didn't provide
     * a key ID
     */
    if (key_id == NULL) {
        /* Create a local copy of the key_filename... */
        loc_key_filename = malloc(strlen(key_filename) + 1);
        if (!loc_key_filename) {
            fprintf(stderr,
                    "ERROR (get_id): can't alloc. local key_filename\n");
            return key_id_buffer;
        }
        strcpy(loc_key_filename, key_filename);
        /**
         * ...discard the path to the basename, and strip any ".pem",
         * ".private.pem" or ".public.pem" extensions from the filename
         */
        loc_key_name = basename(loc_key_filename);
        rchop(loc_key_name, ".private.pem");
        rchop(loc_key_name, ".public.pem");
        rchop(loc_key_name, ".pem");

        if (strlen(loc_key_name) < sizeof(key_id_buffer)) {
            strcpy(key_id_buffer, loc_key_name);
            key_id = key_id_buffer;
        } else {
            fprintf(stderr, "ERROR: Key ID (from key filename) too long\n");
            key_id = NULL;
        }

        /* Smokey the Bear says: "Only you can prevent memory leaks" */
        free(loc_key_filename);
    }

    return key_id;
}


/**
 * @brief Assemble the key name from provided fragments
 *
 * @param buffer A buffer in which to assemble the key name
 * @param length The length of buffer
 * @param key_id The ID string for the key (left of the '@')
 * @param key_domain The string to the right of the '@')
 *
 *
 * @returns A pointer to the passphrase on success, NULL on failure.
 */
bool format_key_name(char * buffer,
                     const size_t length,
                     const char * key_id,
                     const char * key_domain) {
    size_t key_name_length;
    bool success = false;

    /* validate the args */
    if (!buffer || (length < SIGNATURE_KEY_NAME_LENGTH) ||
        !key_filename || !key_domain) {
        /* (we should never get here) */
        errno = EINVAL;
    } else {
        /**
         * The key name specified in the generated signature is generated from
         * the key ID (by default, derived from the key file name), an @
         * symbol, and the supplied key domain.
         */
        key_name_length = strlen(key_id) + 1 + strlen(key_domain);
        if (key_name_length < SIGNATURE_KEY_NAME_LENGTH) {
            snprintf(buffer, length, "%s@%s", key_id, key_domain);
            success = true;
        } else {
            fprintf(stderr, "ERROR: Key Name is too long (%u > %u)\n",
                    (uint32_t)key_name_length, SIGNATURE_KEY_NAME_LENGTH - 1);
        }
    }

    return success;
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
 * @brief Entry point for the sign-tftf application
 *
 * @param argc The number of elements in argv or parsed_argv (std. unix argc)
 * @param argv The unix argument vector - an array of pointers to strings.
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
int main(int argc, char * argv[]) {
    bool success = true;
    bool bad_passphrase = false;
    struct argparse * parse_tbl = NULL;
    int program_status = PROGRAM_SUCCESS;
    char key_name[SIGNATURE_KEY_NAME_LENGTH];

    /* Parse the command line arguments */
    parse_tbl = new_argparse(parse_table, argv[0], NULL, NULL, "<file>...", NULL);
    if (parse_tbl) {
        success =  parse_args(argc, argv, all_args, parse_tbl);
        if (!success) {
            program_status = parser_help? PROGRAM_SUCCESS : PROGRAM_ERRORS;
        }
        parse_tbl = free_argparse(parse_tbl);
    } else {
        success = false;
    }


    /* Perform any argument validation/post-processing */
    if (success) {
        /* Ensure we have a key ID */
        key_id = get_id(key_id, key_filename);
        if (!key_id) {
            fprintf(stderr, "ERROR: No Key ID\n");
            success = false;
        } else {
            /* Generate the key name from the supplied bits */
            success = format_key_name(key_name,
                                      sizeof(key_name),
                                      key_id,
                                      key_domain);
        }
    }

    /* Verify the user has provided us with something to sign */
    if (optind >= argc) {
        fprintf(stderr, "ERROR: No TFTF files to sign\n");
        success = false;
    }

    /* Now is the time to prompt for the password for "--pass prompt"*/
    if (success && (passin_mode == PASSIN_PROMPT)) {
        do {
            passphrase = get_passphrase(passin_mode);
            if (!passphrase) {
                fprintf(stderr, "ERROR: Missing passphrase for private key %s\n",
                        key_filename);
                success = false;
            }

            /* Check that the passphrase is good */
            bad_passphrase = false;
            if (sign_init(key_filename, &bad_passphrase)) {
                sign_deinit();
            }
            if (bad_passphrase) {
               if (no_retry_flag) {
                    fprintf(stderr, "ERROR: Invalid passphrase\n");
                    success = false;
                    break;
                } else {
                    fprintf(stderr, "Sorry, invalid passphrase. Try again\n");
                }
            }
        } while(success && bad_passphrase);
    }


    /* Process the remaining command line arguments as files to sign. */
    if (success) {
        if (sign_init(key_filename, &bad_passphrase)) {
            for (; optind < argc; optind++) {
                if (!sign_tftf(argv[optind],
                               signature_algorithm,
                               key_name,
                               key_filename,
                               !check_flag,
                               verbose_flag)) {
                    fprintf(stderr, "ERROR: Unable to sign %s\n",
                            argv[optind]);
                    success = false;
                }
            }
            sign_deinit();
        } else {
            fprintf(stderr, "ERROR: Couldn't initialize crypto\n");
            success = false;
        }
    }

    if (!success) {
        program_status = PROGRAM_ERRORS;
    }

    return program_status;
}
