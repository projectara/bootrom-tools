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
 * @brief: This file contains headers for the common parsing support code
 *
 */

#ifndef _COMMON_PARSE_SUPPORT_H
#define _COMMON_PARSE_SUPPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

struct optionx;

typedef void (*PreprocessCallback)(const int option);

typedef bool (*OptionCallback)(const int option,
                               const char * optarg,
                               struct optionx * optx);

struct optionx {
    const int               short_name;
    const char *            name;
    void *                  var_ptr;
    const int               default_val;
    const uint32_t          flags;
    const OptionCallback    callback;
    int                     count;  /* # of times this arg was encountered */
};


/* optionx flags values */
#define REQUIRED        BIT(0)
#define DEFAULT_VAL     BIT(1)
#define STORE_FALSE     BIT(2)
#define STORE_TRUE      BIT(3)


struct argparse {
    int                 num_entries;
    PreprocessCallback  preprocess;
    struct optionx *    optx;
    struct option *     opt;
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
bool parse_args_init(struct optionx * optx,
                     struct option * opt,
                     uint32_t count);


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
                               PreprocessCallback preprocess);


/**
 * @brief Deallocate an argparse structure
 *
 * @param optx Pointer to the src optionx table
 *
 * @returns NULL as a convenience to allow the caller to assign it to the
 *          supplied pointer
 */
struct argparse * free_argparse(struct argparse *argp);


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
                struct argparse *parse_table);


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
bool store_hex(const int option, const char * optarg, struct optionx * optx);


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
bool store_str(const int option, const char * optarg, struct optionx * optx);


/**
 * @brief Parse a hex number.
 *
 * @param optarg The (hopefully) numeric string to parse (argv[i])
 * @param optname The name of the argument, used for error messages.
 * @param num Points to the variable in which to store the parsed number.
 *
 * @returns Returns true on success, false on failure
 */
extern bool get_num(const char * optarg, const char * optname, uint32_t * num);


/**
 * @brief Parse an FFFF ElementType/TFTF PackageType.
 *
 * @param optarg The string to parse (argv[i])
 * @param type Points to the variable in which to store the parsed type.
 *
 * @returns Returns true on success, false on failure
 */
bool get_type(const char * optarg, uint32_t * type);

#endif /* _COMMON_PARSE_SUPPORT_H */

