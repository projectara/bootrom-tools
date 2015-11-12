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


typedef struct {
    const char *   string;
    const uint32_t value;
} parse_entry;

#define TOKEN_NOT_FOUND ((uint32_t)-1)

struct optionx;

typedef void (*PreprocessCallback)(const int option);

typedef bool (*OptionCallback)(const int option,
                               const char * optarg,
                               struct optionx * optx);

/* Describes (how to parse) a single command line argument */
struct optionx {
    /* 1-character short name for the arg, used in the "-x" form */
    const int       short_name;

    /* Long name for the argument, used for the "--xxx" form */
    char *          name;

    /* (optional) Name for the optarg value  (e.g., --yabba=foo)  */
    const char *    val_name;

    /* Pointer to the variable which will hold the value */
    void *          var_ptr;

    /* Default value to assign to var_ptr */
    int             default_val;

    /* Parsing flags */
    uint32_t        flags;

    /**
     * Pointer to the parsing callback function. This is given the short_name
     * and the optarg string and a pointer to this structure.
     */
    OptionCallback  callback;

    /* The number of times this arg was encountered */
    int             count;/* # of times this arg was encountered */

    /* (optional) Help string for this arguments   */
    const char *            help;
};


/* optionx.flags values */
#define OPTIONAL        0
#define REQUIRED        BIT(0)
#define DEFAULT_VAL     BIT(1)
#define STORE_FALSE     BIT(2)
#define STORE_TRUE      BIT(3)


struct argparse {
    /* The name of the program (typically argv.[0]) */
    const char *        prog;

    /* (optional) Text to display before argument help */
    const char *        description;

    /* (optional) Text to display after argument help */
    const char *        epilog;

    /* (optional) Text to display at the end of the usage string for positional args */
    const char *        positional_arg_description;

    /* The number of entries in optionx and option */
    int                 num_entries;

    /* The number of secondary entries */
    int                 num_secondary_entries;

    /**
     * (optional) Function to call with the current option before the option
     * is dispatched to the appropriate OptionCallback()
     */
    PreprocessCallback  preprocess;

    /* The parse description table - an array of optionx's, with an all-zero
     * entry as an end-of-table marker.
    */
    struct optionx *    optx;

    /**
     *  The allocated option array generated from optx and passed to
     *  getopt_long()
     */
    struct option *     opt;
};


/**
 * @brief Create an option table from an optionx table
 *
 * @param argp Pointer to the parsing context
 *
 * @returns true on success, false on failure
 */
bool parse_args_init(struct argparse * argp);


/**
 * @brief Create an argparse structure, initialized from an optionx table
 *
 * @param optx Pointer to the src optionx table
 * @param prog The name of the program (typically argv.[0])
 * @param description (optional) Text to display before argument help
 * @param epilog (optional) Text to display after argument help
 * @param positional_arg_description (optional) Text to display at the end
 *        of the usage string for positional args
 * @param preprocess (optional) Pointer to a function which performs some
 *        check of the current "option" character before parse_args processes
 *        it further. (Typically used to close a multi-arg window.)
 *
 * @returns A pointer to the allocated struct on success, NULL on failure
 */
struct argparse * new_argparse(struct optionx *optx,
                               const char * prog,
                               const char * description,
                               const char * epilog,
                               const char * positional_arg_description,
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
 * @brief Print a usage message from the argparse info
 *
 * @param argp Pointer to the parsing context
 *
 * @returns Nothing
 */
void usage(struct argparse *argp);


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
 * @brief Generic parsing callback to store a flag.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool store_flag(const int option, const char * optarg, struct optionx * optx);


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
 * @brief Simple keyword lookup.
 *
 * @param keyword The string to parse (argv[i])
 * @param token Points to the variable in which to store the parsed type.
 *
 * @returns Returns the found token on success, TOKEN_NOT_FOUND on failure
 */
uint32_t kw_to_token(const char * keyword, const parse_entry * lookup);


/**
 * @brief Simple keyword reverse lookup.
 *
 * @param keyword The string to parse (argv[i])
 * @param token Points to the variable in which to store the parsed type.
 *
 * @returns Returns the string on success, NULL on failure
 */
const char * token_to_kw(const uint32_t token, const parse_entry * lookup);


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

