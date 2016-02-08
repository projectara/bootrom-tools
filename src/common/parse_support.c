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
#include <libgen.h>
#include "util.h"
#include "ffff.h"
#include "parse_support.h"

/* Maximum length of a usage line to be displayed */
#define USAGE_LINE_LENGTH 80

bool parser_help = false;
bool parser_invalid_arg = false;


static parse_entry element_types[] = {
    {"s2fw", FFFF_ELEMENT_STAGE_2_FW},
    {"s3fw", FFFF_ELEMENT_STAGE_3_FW},
    {"icert", FFFF_ELEMENT_IMS_CERT},
    {"ccert", FFFF_ELEMENT_CMS_CERT},
    {"data", FFFF_ELEMENT_DATA},
    {"end", FFFF_ELEMENT_END},
    {NULL, 0}
};


/**
 * @brief Create an option table from an optionx table
 *
 * @param opt Pointer to the dst option entry to initialize
 * @param name Pointer to the name to use for the option
 * @param optx Pointer to the src optionx entry
 *
 * @returns Nothing
 */
void parse_args_init_opt_entry(struct option * opt, const char * name,
                               struct optionx * optx) {
    opt->val = optx->short_name;
    opt->name = name;
    opt->flag = NULL;

    if (optx->flags & (STORE_FALSE | STORE_TRUE)) {
        opt->has_arg = no_argument;
    } else {
        opt->has_arg = required_argument;
    }
}


/**
 * @brief Create an option table from an optionx table
 *
 * @param argp Pointer to the parsing context
 *
 * @returns true on success, false on failure
 */
bool parse_args_init(struct argparse * argp) {
    struct option * opt;
    struct optionx * optx;
    int index;

    if (!argp || !argp->optx || !argp->opt) {
        fprintf(stderr, "ERROR (getoptionx_init): Invalid args\n");
        return false;
    }

    opt = argp->opt;
    optx = argp->optx;

    /* Process the primary names */
    for (optx = argp->optx; optx->long_names != NULL; optx++, opt++) {
        if ((optx->long_names)[0]) {
            /* Normal case */
            parse_args_init_opt_entry(opt, (optx->long_names)[0], optx);

            if (optx->flags & (STORE_FALSE | STORE_TRUE)) {
                optx->flags |= DEFAULT_VAL;
                optx->default_val = optx->flags & (STORE_FALSE)? true : false;
                if (!optx->callback) {
                    optx->callback = &store_flag;
                }
            }

            /* Initialize variable portions of the optx entry */
            optx->count = 0;
        }
    }

    /**
     * Process the secondary names.
     * What we will do is add additional "opt" entries, each pointing
     * into the additional names in the long_names array
     */
    if (argp->num_secondary_entries > 0) {
        for (optx = argp->optx; optx->long_names != NULL; optx++) {
            index = 1;
            for (index=1; (optx->long_names)[index] != NULL; index++, opt++) {
                parse_args_init_opt_entry(opt, (optx->long_names)[index], optx);
            }
        }
    }

    /* Finally, add the End-Of-Table marker to the opt array */
    opt->name = NULL;
    opt->has_arg = 0;
    opt->flag = NULL;
    opt->val = 0;

    return true;
}


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
                               PreprocessCallback preprocess) {
    struct optionx *scan = optx;
    int num_entries = 0;
    int num_secondary_entries = 0;
    int index;
    struct argparse * argp = NULL;

    if (optx && prog) {
        /**
         *  Count the number of entries in optx, including the terminating entry
         */
        while (true) {
            num_entries++;
            if (scan->long_names == NULL) {
                break;
            } else {
                /* Count the number of secondary name strings */
                for (index = 1; (scan->long_names)[index] != NULL; index++) {
                    num_secondary_entries++;
                }
            }
            scan++;
        }

        /* Allocate the argparse and create its opt table */
        argp = malloc(sizeof(*argp));
        if (!argp) {
            fprintf(stderr, "ERROR(new_argparse): Can't allocate argparse\n");
        } else {
            argp->prog = prog;
            argp->description = description;
            argp->epilog = epilog;
            argp->positional_arg_description = positional_arg_description;
            argp->num_entries = num_entries;
            argp->num_secondary_entries = num_secondary_entries;
            argp->preprocess = preprocess;
            argp->optx = optx;
            argp->opt = calloc(num_entries + num_secondary_entries,
                               sizeof(struct option));
            if (!argp->opt) {
                fprintf(stderr,
                        "ERROR(new_argparse): Can't allocate option table\n");
                free(argp);
                argp = NULL;
            } else {
                parse_args_init(argp);
            }
        }
    }

    return argp;
}


/**
 * @brief Deallocate an argparse structure
 *
 * @param argp Pointer to the parsing context
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
    int total_entries;
    struct optionx *optx = NULL;
    bool success = true;

    total_entries = parse_table->num_entries +
            parse_table->num_secondary_entries;

    /**
     * Suppress the builtin "unrecognized option" so we can preprocess for "--help"
     */
    opterr = 0;

    /* Pre-parsing, apply defaults */
    for (optx = parse_table->optx; (optx->long_names) != NULL; optx++) {
        if (optx->count == 0) {
            /* Missing arg */
            if ((optx->flags & DEFAULT_VAL) && optx->var_ptr) {
                *(uint32_t*)optx->var_ptr = optx->default_val;
            }
        }
    }

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

        /* Unrecognized option? */
        if (option == '?') {
            char * offending_arg = argv[optind - 1];
            if (strcmp(offending_arg, "--help") == 0) {
                parser_help = true;
                usage(parse_table);
            } else {
                parser_invalid_arg = true;
                fprintf(stderr, "%s: unrecognized option '%s'\n\n",
                        basename(argv[0]), offending_arg);
                parser_help = true;
                usage(parse_table);
            }
            success = false;
            continue;
        }

        /**
         * Perform any global pre-processing before calling the appropriate callback.
         */
        if (parse_table->preprocess) {
            parse_table->preprocess(option);
        }


       /* Map secondary names into primary names */
       if ((option != 0) &&
           (option_index >= parse_table->num_entries) &&
           (option_index < total_entries)) {
           /**
            * The index belongs to one of the secondary names, scan through
            * the primary names for a matching option character
            */
           for (option_index = 0; option_index < parse_table->num_entries; option_index++) {
               if (option == parse_table->optx[option_index].short_name) {
                   break;
               }
           }
       }


       /* Process the option */
       if ((option != 0) &&
           (option_index >= 0) &&
           (option_index < parse_table->num_entries)) {
           struct optionx *optx = &parse_table->optx[option_index];

           /**
            *  Workaround for getopt_long's handling of args w/o values
            *  (e.g., flags).
            *  When getop_long encounters a short-named arg with,
            *  "opt->has_arg = no_argument", it returns an option_index of
            *  zero. The workaround is to check the option char against the
            *  short name, and if they don't match, search for it ourselves.
            */
           if (option != optx->short_name) {
               for (optx = parse_table->optx; (optx->long_names) != NULL; optx++) {
                   if (optx->short_name == option) {
                       break;
                   }
               }
           }

           if (optx->callback) {
               optx->count++;
               if (!optx->callback(option, optarg, optx)) {
                   success = false;
               }
           }
        }
    }

    if (success) {
        /* Post-parsing, squawk about missing params */
        for (optx = parse_table->optx; (optx->long_names) != NULL; optx++) {
            if (optx->count == 0) {
                /* Missing arg */
                if (optx->flags & REQUIRED) {
                    fprintf (stderr, "ERROR: --%s is required\n",
                            (optx->long_names)[0]);
                    success = false;
                }
            }
        }
    }

    return success;
}


/**
 * @brief Generic parsing callback to store a hex number.
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
        return get_num(optarg, (optx->long_names)[0], (uint32_t *)optx->var_ptr);
    } else {
        fprintf(stderr, "ERROR: No var to store --%s", (optx->long_names)[0]);
        return false;
    }
}


/**
 * @brief Generic parsing callback to store a string.
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
        fprintf(stderr, "ERROR: No var to store --%s", (optx->long_names)[0]);
        return false;
    }
}


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
bool store_flag(const int option, const char * optarg, struct optionx * optx) {
    if (optx->var_ptr) {
        *(int*)optx->var_ptr = (optx->flags & STORE_TRUE)? true : false;
        return true;
    } else {
        fprintf(stderr, "ERROR: No var to store --%s", (optx->long_names)[0]);
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

    *num = strtoul(optarg, &tail, 0);
    if ((errno != errno) || ((tail != NULL) && (*tail != '\0'))) {
        fprintf (stderr, "Error: invalid %s '%s'\n", optname, optarg);
        success = false;
    }

    return success;
}


/**
 * @brief Simple keyword lookup.
 *
 * @param keyword The string to parse (argv[i])
 * @param token Points to the variable in which to store the parsed type.
 *
 * @returns Returns the found token on success, TOKEN_NOT_FOUND on failure
 */
uint32_t kw_to_token(const char * keyword, const parse_entry * lookup) {
    uint32_t token = TOKEN_NOT_FOUND;
    int i;

    if (keyword && lookup) {
        /* While this is a simple linear search, N is typically < 10 */
        for (; lookup->string != NULL; lookup++) {
            if (strcmp(keyword, lookup->string) == 0) {
                token = lookup->value;
                break;
            }
        }
    }
    return token;
}


/**
 * @brief Simple keyword reverse lookup.
 *
 * @param keyword The string to parse (argv[i])
 * @param token Points to the variable in which to store the parsed type.
 *
 * @returns Returns the string on success, NULL on failure
 */
const char * token_to_kw(const uint32_t token, const parse_entry * lookup) {
    const char * keyword = NULL;

    if (lookup) {
        /* While this is a simple linear search, N is typically < 10 */
        for (; lookup->string != NULL; lookup++) {
            if (token == lookup->value) {
                keyword = lookup->string;
                break;
            }
        }
    }
    return keyword;
}


/**
 * @brief Parse an FFFF ElementType/TFTF PackageType.
 *
 * @param type_name The string to parse (argv[i])
 * @param type Points to the variable in which to store the parsed type.
 *
 * @returns Returns true on success, false on failure
 */
bool get_type(const char * type_name, uint32_t * type) {
    int i;

    *type = kw_to_token(type_name, element_types);
    return *type != TOKEN_NOT_FOUND;
}


/**
 * @brief Print a single option's usage message
 *
 * @param optx Pointer to the opion to print
 *
 * @returns Nothing
 */
void usage_arg(struct optionx *optx) {
    int index;

    fprintf(stderr, "  -%c", optx->short_name);
    for (index = 0; (optx->long_names)[index] != NULL; index++) {
        fprintf(stderr, " | --%s", (optx->long_names)[index]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "    %s\n", optx->help);
}


/**
 * @brief Print a usage message from the argparse info
 *
 * Automatically generates the usage message in a style inspired by Python's
 * "argparse"
 *
 * @param argp Pointer to the parsing context
 *
 * @returns Nothing
 */
void usage(struct argparse *argp) {
    if (argp) {
        char item_buf[256];
        int line_length;
        int item_length;
        struct optionx * optx;
        bool issued_header;

        /* Print the usage string */
        line_length = fprintf(stderr, "usage: %s ", argp->prog);
        for (optx = argp->optx; (optx->long_names) != NULL; optx++) {

            /* Format this argument */
            item_length = snprintf(item_buf, sizeof(item_buf),
                                   " [--%s%s%s]",
                                   (optx->long_names)[0],
                                   (optx->val_name)? " " : "",
                                   (optx->val_name)? optx->val_name : "");
            if ((line_length + item_length) >= USAGE_LINE_LENGTH) {
                fprintf(stderr, "\n");
                line_length = 0;
            }
            fprintf(stderr, "%s", item_buf);
            line_length += item_length;
        }
        /* Optionally add the nargs section */
        if (argp->positional_arg_description) {
            item_length = strlen(argp->positional_arg_description);
            if ((line_length + item_length) >= USAGE_LINE_LENGTH) {
                fprintf(stderr, "\n");
                line_length = 0;
            }
            fprintf(stderr, "%s", argp->positional_arg_description);
            line_length += item_length;
        }
        /* Close off the last line of usage */
        if (line_length != 0) {
            fprintf(stderr, "\n");
        }

        /* Optionally print the description */
        if (argp->description) {
            fprintf(stderr, "\n%s\n", argp->description);
        }

       /* Print the (required) argument help */
        for (optx = argp->optx, issued_header = false;
             (optx->long_names) != NULL;
             optx++) {
            if (optx->flags & REQUIRED) {
                if(!issued_header) {
                    fprintf (stderr, "\narguments:\n");
                    issued_header = true;
                }
                usage_arg(optx);
            }
        }

        /* Print the (optional) argument help */
        for (optx = argp->optx, issued_header = false;
              (optx->long_names) != NULL;
              optx++) {
            if (!(optx->flags & REQUIRED)) {
                if(!issued_header) {
                    fprintf (stderr, "\noptional arguments:\n");
                    issued_header = true;
                }
                usage_arg(optx);
            }
        }

        /* Optionally print the epilog */
        if (argp->epilog) {
            fprintf(stderr, "\n%s\n", argp->epilog);
        }
    }
}

