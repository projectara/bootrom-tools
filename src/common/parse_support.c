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
 * @brief: This file contains common parsing support code
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include "util.h"
#include "ffff.h"
#include "parse_support.h"


typedef struct {
    const char *   string;
    const uint32_t value;
} parse_entry;


static parse_entry element_types[] = {
    {"s3fw", FFFF_ELEMENT_STAGE_2_FW},
    {"s2fw", FFFF_ELEMENT_STAGE_3_FW},
    {"icert", FFFF_ELEMENT_IMS_CERT},
    {"ccert", FFFF_ELEMENT_CMS_CERT},
    {"data", FFFF_ELEMENT_DATA},
    {"end", FFFF_ELEMENT_END},
};


/**
 * @brief Create an option table from an optionx table
 *
 * @param optx Pointer to the src optionx table
 * @param opg Pointer to the dst option table
 * @param count The number of entries in both tables
 *
 * @returns true on success, false on failure
 */
bool parse_args_init(struct optionx * optx, struct option * opt,
                     uint32_t count) {
    struct optionx * optx_end;

    if (!optx || !opt) {
        fprintf(stderr, "ERROR (getoptionx_init): Invalid args\n");
        return false;
    }

    for (optx_end = &optx[count]; optx < optx_end; optx++, opt++) {
        /* End of table? */
        if (!optx->name) {
            opt->name = NULL;
            opt->has_arg = 0;
            opt->flag = NULL;
            opt->val = 0;
            break;
        }

        /* Normal case */
        opt->name = optx->name;
        if ((optx->flags & STORE_FALSE) || (optx->flags & STORE_TRUE)) {
            opt->has_arg = no_argument;
            opt->flag = (int *)optx->var_ptr;
            opt->val = (optx->flags & STORE_TRUE)? 1 : 0;
        } else {
            opt->has_arg = required_argument;
            opt->flag = NULL;
            opt->val = optx->short_name;
        }

        /* Initialize variable portions of the optx entry */
        optx->count = 0;
    }

    return true;
}


/**
 * @brief Create an argparse structure, initialized from an optionx table
 *
 * @param optx Pointer to the src optionx table
  * @param preprocess Optional pointer to a function which performs some
 *        check of the current "option" character before parse_args processes
 *        it further. (Typically used to close a multi-arg window.)
 *
 * @returns A pointer to the allocated struct on success, NULL on failure
 */
struct argparse * new_argparse(struct optionx *optx,
                               PreprocessCallback preprocess) {
    struct optionx *scan = optx;
    int num_entries = 0;
    struct argparse * argp = NULL;

    if (optx != NULL) {
        /**
         *  Count the number of entries in optx, including the terminating entry
         */
        while (true) {
            num_entries++;
            if (scan->name == NULL) {
                break;
            }
            scan++;
        }

        /* Allocate the argparse and create its opt table */
        argp = malloc(sizeof(*argp));
        if (!argp) {
            fprintf(stderr, "ERROR(new_argparse): Can't allocate argparse\n");
        } else {
            argp->num_entries = num_entries;
            argp->preprocess = preprocess;
            argp->optx = optx;
            argp->opt = calloc(num_entries, sizeof(struct option));
            if (!argp->opt) {
                fprintf(stderr,
                        "ERROR(new_argparse): Can't allocate option table\n");
                free(argp);
            } else {
                parse_args_init(argp->optx, argp->opt, argp->num_entries);
            }
        }
    }
    return argp;
}


/**
 * @brief Deallocate an argparse structure
 *
 * @param optx Pointer to the src optionx table
 *
 * @returns NULL as a convenience to allow the caller to assign it to the
 *          supplied pointer
 */
struct argparse * free_argparse(struct argparse *argp) {
    if (argp) {
        free(argp->opt);
        free(argp);
    }
    return NULL;
}


/**
 * @brief Parse all of the args
 *
 * @param argc Count of command line args
 * @param argv The command line argument vector
 * @param optstring A string containing the legitimate option characters
 * @param parse_table The parsing table
 *
 * @returns True if there were no errors, false if there were
 */
bool parse_args(int argc, char * const argv[], const char *optstring,
                struct argparse *parse_table) {
    int option;
    int option_index;
    struct optionx *optx = NULL;
    bool success = true;

    /* Parsing loop */
    while (true) {
        option = getopt_long_only (argc,
                          argv,
                          optstring,
                          parse_table->opt,
                          &option_index);
        /* End of options? */
        if (option == -1) {
            break;
        }

        /**
         * Close the window on the current section whenever we see a
         * non-section arg
         */
       if (parse_table->preprocess) {
            parse_table->preprocess(option);
        }

        /* Process the option */
       if ((option != 0) && (option_index < parse_table->num_entries)) {
            struct optionx *optx = &parse_table->optx[option_index];
           if (optx->callback) {
                optx->count++;
               optx->callback(option, optarg, optx);
           }
        }
    }

    /* Post-parsing, apply defaults or squawk about missing params */
    for (optx = parse_table->optx; optx->name != NULL; optx++) {
        if (optx->count == 0) {
            /* Missing arg */
            if (optx->flags & REQUIRED) {
                fprintf (stderr, "ERROR: --%s is required\n", optx->name);
                success = false;
            } else if ((optx->flags & DEFAULT_VAL) && optx->var_ptr) {
                *(uint32_t*)optx->var_ptr = optx->default_val;
            }
        }
    }
    return success;
}


/**
 * @brief Generic callback to store a hex number.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool store_hex(const int option, const char * optarg, struct optionx * optx) {
    if (optx->var_ptr) {
        return get_num(optarg, optx->name, (uint32_t *)optx->var_ptr);
    } else {
        fprintf(stderr, "ERROR: No var to store --%s", optx->name);
        return false;
    }
}


/**
 * @brief Generic callback to store a string.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool store_str(const int option, const char * optarg, struct optionx * optx) {
    if (optx->var_ptr) {
        *(const char**)optx->var_ptr = optarg;
        return true;
    } else {
        fprintf(stderr, "ERROR: No var to store --%s", optx->name);
        return false;
    }
}


/**
 * @brief Parse a hex number.
 *
 * @param optarg The (hopefully) numeric string to parse (argv[i])
 * @param optname The name of the argument, used for error messages.
 * @param num Points to the variable in which to store the parsed number.
 *
 * @returns Returns true on success, false on failure
 */
bool get_num(const char * optarg, const char * optname, uint32_t * num) {
    char *tail = NULL;
    bool success = true;

    *num = strtoul(optarg, &tail, 16);
    if ((errno != errno) || ((tail != NULL) && (*tail != '\0'))) {
        fprintf (stderr, "Error: invalid %s '%s'\n", optname, optarg);
        success = false;
    }

    return success;
}


/**
 * @brief Parse an FFFF ElementType/TFTF PackageType.
 *
 * @param optarg The string to parse (argv[i])
 * @param type Points to the variable in which to store the parsed type.
 *
 * @returns Returns true on success, false on failure
 */
bool get_type(const char * optarg, uint32_t * type) {
    int i;

    for (i = 0; i < _countof(element_types); i++) {
        if (strcmp(optarg, element_types[i].string) == 0) {
            *type = element_types[i].value;
            return true;
        }
    }
    return false;
}

