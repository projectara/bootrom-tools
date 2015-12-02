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
 * @brief: This file contains the code for "create-tftf" a Linux command-line
 * app used to create an unsigned Trusted Firmware Transfer Format (TFTF)
 * file used by the secure bootloader.
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
const char * output_filename = NULL;
uint32_t    header_size = TFTF_HEADER_SIZE;
const char *fw_pkg_name = NULL;
uint32_t    package_type = FFFF_ELEMENT_STAGE_3_FW;
uint32_t    start_location;
uint32_t    unipro_mfg;
uint32_t    unipro_pid;
uint32_t    ara_vid;
uint32_t    ara_pid;
int         verbose_flag = false;
int         map_flag = false;

char outfile_path[MAXPATH];

/**
 * Flag (effective only when allow_section_parameters is True) indicating that
 * the --load option is disallowed for that section type.
 */
bool        restricted_address = false;

/**
 * Flag indicating that a restricted section (i.e., signature or certificate)
 * has been parsed. No data or code sections are allowed to follow them.
 */
bool        code_data_blocked = false;

/**
 * Flag indicating that the user specified a code or data section after a
 * restricted section.
 */
bool        code_data_while_blocked = false;

static char * warn_load_ignored =
    "Warning: --load is ignored for --signature, --certificate";

/**
 * If compress is true, compress the data while copying; if false, just
 * perform a raw copy. (Ignored if COMPRESSED_SUPPORT is not defined.)
 */
int compress_flag = false;


/* TFTF-specific parsing callbacks */
bool handle_header_size(const int option, const char * optarg,
        struct optionx * optx);
bool handle_header_type(const int option, const char * optarg,
        struct optionx * optx);
bool handle_section_elf(const int option, const char * optarg,
        struct optionx * optx);
bool handle_section_normal(const int option, const char * optarg,
        struct optionx * optx);
bool handle_section_class(const int option, const char * optarg,
        struct optionx * optx);
bool handle_section_id(const int option, const char * optarg,
        struct optionx * optx);
bool handle_section_load_address(const int option, const char * optarg,
        struct optionx * optx);

static char * header_size_names[] = { "header-size", NULL };
static char * fw_pkg_name_names[] = { "type", NULL };
static char * package_type_names[] = { "name", NULL };
static char * start_location_names[] = { "start", NULL };
static char * unipro_mfg_names[] = { "unipro-mfg", NULL };
static char * unipro_pid_names[] = { "unipro-pid", NULL };
static char * ara_vid_names[] = { "ara-vid", NULL };
static char * ara_pid_names[] = { "ara-pid", NULL };
static char * ara_stage_names[] = { "ara-stage", NULL };
static char * section_elf_names[] = { "elf", NULL };
static char * section_code_names[] = { "code", NULL };
static char * section_data_names[] = { "data", NULL };
static char * section_manifest_names[] = { "manifest", NULL };
static char * section_signature_names[] = { "signature", NULL };
static char * section_certificate_names[] = { "certificate", NULL };
static char * section_class_names[] = { "class", NULL };
static char * section_id_names[] = { "id", NULL };
static char * section_load_address_names[] = { "load", "load-address", NULL };
static char * output_filename_names[] = { "out", NULL };
static char * verbose_flag_names[] = { "verbose", NULL };
static char * map_flag_names[] = { "map", NULL };

char ** foo = header_size_names;

/* Parsing table */
static struct optionx parse_table[] = {
    /* Header args */
    { 'z', header_size_names, "num", &header_size, TFTF_HEADER_SIZE,
      DEFAULT_VAL, &handle_header_size, 0,
      "The size of the generated TFTF header, in bytes (512)"
    },
    { 't', fw_pkg_name_names, "s2fw | s3fw", &package_type, 0,
      REQUIRED, &handle_header_type, 0,
      "Package type"
    },
    { 'n', package_type_names, "text", &fw_pkg_name, 0,
      OPTIONAL, &store_str, 0,
      "Package name"
    },
    { 's', start_location_names, "address", &start_location, DFLT_START,
      DEFAULT_VAL, &store_hex, 0,
      "The memory location of the package entry point."
    },
    { 'u', unipro_mfg_names, "num", &unipro_mfg, DFLT_UNIPRO_MID,
      DEFAULT_VAL, &store_hex, 0,
      "Unipro ASIC Manufacturer ID"
    },
    { 'U', unipro_pid_names, "num", &unipro_pid, DFLT_UNIPRO_PID,
      DEFAULT_VAL, &store_hex, 0,
      "Unipro ASIC Product ID"
    },
    { 'a', ara_vid_names, "num", &ara_vid, DFLT_ARA_VID,
      DEFAULT_VAL, &store_hex, 0,
      "Ara Vendor ID"
    },
    { 'A', ara_pid_names, "num", &ara_pid, DFLT_ARA_PID,
      DEFAULT_VAL, &store_hex, 0,
      "Ara Product ID"
    },

    /* Section args */
    { 'E', section_elf_names, NULL, NULL, 0,
      OPTIONAL, &handle_section_elf, 0,
      "The name of an input ELF image file (extracts -C, -D and -s)"
    },
    { 'C', section_code_names, NULL, NULL, 0,
      OPTIONAL, &handle_section_normal, 0,
      "Code section [1]"
    },
    { 'D', section_data_names, NULL, NULL, 0,
      OPTIONAL, &handle_section_normal, 0,
      "Data  section [1]"
    },
    { 'M', section_manifest_names, NULL, NULL, 0,
      OPTIONAL, &handle_section_normal, 0,
      "Manifest section [1]"
    },
    { 'G', section_signature_names, NULL, NULL, 0,
      OPTIONAL, &handle_section_normal, 0,
      "Signature section [1]"
    },
    { 'R', section_certificate_names, NULL, NULL, 0,
      OPTIONAL, &handle_section_normal, 0,
      "Certificate section [1]"
    },
    { 'c', section_class_names, "num", NULL, DFLT_SECT_CLASS,
      DEFAULT_VAL, &handle_section_class, 0,
      "Set the section class to <num>"
    },
    { 'i', section_id_names, "num", NULL, DFLT_SECT_ID,
      DEFAULT_VAL, &handle_section_id, 0,
      "Set the section id to <num>"
    },
    { 'l', section_load_address_names, "num", NULL, DFLT_SECT_LOAD,
      DEFAULT_VAL, &handle_section_load_address, 0,
      "Set the address of the start of the section to <num>"
    },

    /* Misc args */
    { 'o', output_filename_names, "file", &output_filename, 0,
      REQUIRED, &store_str, 0,
      "Specifies the output file"
    },
    { 'v', verbose_flag_names, NULL, &verbose_flag, 0,
      DEFAULT_VAL | STORE_TRUE, &store_flag, 0,
      "Display the TFTF header and a synopsis of each TFTF section"
    },
    { 'm', map_flag_names, NULL, &map_flag, 0,
      DEFAULT_VAL | STORE_TRUE, &store_flag, 0,
      "Create a map file of the TFTF header and each TFTF section"
    },
    { 0, NULL, NULL, NULL, 0, 0, NULL, 0 , NULL}
};


/* The 1-char tags for all args */
static char * all_args = "z:n:t:s:u:U:a:A:E:C:D:M:G:R:c:i:l:o:vm";

/**
 * The 1-char tags for each of the section-related args
 * (close_section_if_needed uses this to filter)
 */
static char * section_args = "ECDMGRcil";


static char * epilog =
    "NOTES:\n"
    "  1. sections are specified as [<section_type> <section_option>]...\n"
    "     <section_type> ::= [--code | --data | --manifest | --certificate |\n"
    "                       --signature]\n"
    "     <section_option> ::= {--load} {--class} {--id}\n"
    "  2. --code and --data cannot follow --signature or --certificate\n"
    "  3. --load ignored for --signature or --certificate";


/**
 * @brief Callback to validate and store the TFTF header size.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_header_size(const int option, const char * optarg,
                        struct optionx * optx) {
    bool success;

    success = get_num(optarg, (optx->long_names)[0], &header_size);

    /* Make sure the header size is plausible */
    if ((header_size < TFTF_HEADER_SIZE_MIN) ||
        (header_size > TFTF_HEADER_SIZE_MAX) ||
        !is_power_of_2(header_size)) {
        fprintf(stderr, "ERROR: Header size is out of range (0x%x-0x%x)\n",
                TFTF_HEADER_SIZE_MIN, TFTF_HEADER_SIZE_MAX);
        success = false;
    } else if ((header_size % 4) != 0) {
        fprintf(stderr, "ERROR: Header size must be a multiple of 4\n");
        success = false;
    }

    return success;
}


/**
 * @brief Callback to validate and store the TFTF header type.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_header_type(const int option, const char * optarg,
                        struct optionx * optx) {
    bool success;

    success = get_type(optarg, (uint32_t*)optx->var_ptr);
    if (!success) {
        fprintf(stderr, "ERROR: Invalid --type: %s\n", optarg);
    }
    return success;
}


/**
 * @brief Callback to load an ELF file.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_section_elf(const int option, const char * optarg,
                        struct optionx * optx) {
    return load_elf(optarg, &start_location);
}


/**
 * @brief Callback to load a non-ELF section.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_section_normal(const int option, const char * optarg,
                           struct optionx * optx) {
    bool success;
    bool known_arg = true;

    restricted_address = false;

    switch (option) {
    case 'C':   /* code */
        if (code_data_blocked) {
            code_data_while_blocked = true;
            success = false;
        } else {
            success = section_cache_entry_open(TFTF_SECTION_RAW_CODE, optarg) == 0;
        }
        break;

    case 'D':   /* data */
        if (code_data_blocked) {
            code_data_while_blocked = true;
            success = false;
        } else {
            success = section_cache_entry_open(TFTF_SECTION_RAW_DATA, optarg) == 0;
        }
        break;

    case 'G':   /* siGnature */
        /* TODO: We may want to validate the signature we fetch */
        success =
            ((section_cache_entry_open(TFTF_SECTION_SIGNATURE, optarg) == 0) &&
            section_cache_entry_set_load_address(DATA_ADDRESS_TO_BE_IGNORED));
            restricted_address = true;
            code_data_blocked = true;
       break;

    case 'R':   /* ceRtificate */
        /* TODO: We may want to validate the certificate we fetch */
        success =
            ((section_cache_entry_open(TFTF_SECTION_CERTIFICATE,
                                      optarg) == 0) &&
            section_cache_entry_set_load_address(DATA_ADDRESS_TO_BE_IGNORED));
            restricted_address = true;
            code_data_blocked = true;
        break;

    case 'M':   /* manifest */
        success = section_cache_entry_open(TFTF_SECTION_MANIFEST, optarg) == 0;
        break;

    default:
        /* We should never get here */
        success = false;
        known_arg = false;
    }

    if (!success) {
        if (code_data_while_blocked) {
            fprintf(stderr,
                    "ERROR: --%s cannot follow --signature, --certificate\n",
                    (optx->long_names)[0]);
        } else if (known_arg) {
            /* We failed on a known arg. Bad filename? */
            fprintf(stderr, "ERROR: --%s %s failed\n", (optx->long_names)[0], optarg);
        } else {
            fprintf(stderr,
                    "ERROR: unknown section type '%c'\n", option);
        }
    }
    return success;
}


/**
 * @brief Callback to load the section class into the current section.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_section_class(const int option, const char * optarg,
                          struct optionx * optx) {
    uint32_t num;

    return get_num(optarg, (optx->long_names)[0], &num) &&
           section_cache_entry_set_class(num);
}


/**
 * @brief Callback to load the section ID into the current section.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_section_id(const int option, const char * optarg,
                       struct optionx * optx) {
    uint32_t num;

    return get_num(optarg, (optx->long_names)[0], &num) &&
           section_cache_entry_set_id(num);
}


/**
 * @brief Callback to load the section load address into the current section.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_section_load_address(const int option, const char * optarg,
                                 struct optionx * optx) {
    uint32_t num;

    if (restricted_address) {
        fprintf(stderr, "%s\n", warn_load_ignored);
        return false;
    } else {
        return get_num(optarg, (optx->long_names)[0], &num) &&
               section_cache_entry_set_load_address(num);
    }
}


/**
 * @brief Callback to close the section window when we see a non-section arg
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 *
 * @returns Returns nothing
 */
void close_section_if_needed(const int option) {
    if (index(section_args, option) == NULL) {
        restricted_address = false;
        section_cache_entry_close();
    }
}


/**
 * @brief Convert a package_type into a "boot stage" integer
 *
 * @param package_type
 *
 * @returns A corresponding boot stage, -1 if invalid
 */
uint32_t boot_stage(const uint32_t package_type) {
    int boot_stage;

    switch (package_type) {
    case FFFF_ELEMENT_STAGE_2_FW:
        boot_stage = 2;
        break;
    case FFFF_ELEMENT_STAGE_3_FW:
        boot_stage = 3;
        break;
    default:
        boot_stage = (uint32_t)(-1);
        break;
    }
    return boot_stage;
}


/**
 * @brief Sanity-check the command line args and return a "valid" flag
 *
 * @returns Returns true if the parsed args pass muster, false otherwise.
 */
bool validate_args(void) {
    bool success = true;
    uint32_t num_sections;

    num_sections = section_cache_entry_count();
    if (num_sections == 0) {
        fprintf(stderr,
                "ERROR: You need at least one --code, --data, "
                "--manifest, --certificate or --elf\n");
                success = false;

    } else if (num_sections > CALC_MAX_TFTF_SECTIONS(header_size)) {
        fprintf(stderr, "ERROR Too many sections (%u max.\n",
                (unsigned int)CALC_MAX_TFTF_SECTIONS(header_size));
        success = false;
    }

    /* Invent a default filename if needed */
    if (!output_filename) {
        snprintf(outfile_path, sizeof(outfile_path),
                 "ara:%08x:%08x:%08x:%08x:%02x.tftf",
                   unipro_mfg, unipro_pid, ara_vid,
                   ara_pid, boot_stage(package_type));
        output_filename = outfile_path;
    }
    /* TODO: Other checks TBD */
    return success;
}


/**
 * @brief Entry point for the create-tftf application
 *
 * @param argc The number of elements in argv or parsed_argv (std. unix argc)
 * @param argv The unix argument vector - an array of pointers to strings.
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
int main(int argc, char * argv[]) {
    bool success = true;
    tftf_header * tftf_hdr = NULL;
    struct argparse * parse_tbl = NULL;
    int program_status = 0; /* assume success */

    /* Parse the command line arguments */
    parse_tbl = new_argparse(parse_table, argv[0], NULL, epilog, NULL,
                             close_section_if_needed);
    if (parse_tbl) {
        success =  parse_args(argc, argv, all_args, parse_tbl);
    } else {
        success = false;
    }

    if (success) {
        /* Make sure we close off any under-construction section */
        section_cache_entry_close();

        /* Validate that we have the needed args */
        success = validate_args();
    }

    if (!success) {
        usage(parse_tbl);
        program_status = PROGRAM_ERRORS;
    } else {
        /* Calculate the number of sections in this TFTF header */
        /* TODO: Use macro (TBD) in tftf.h to calculate max_sections */
        tftf_max_sections = CALC_MAX_TFTF_SECTIONS(header_size);

        /* Create and initialize the TFTF blob... */
        tftf_hdr = new_tftf(header_size,
                            section_cache_entries_size(),
                            fw_pkg_name,
                            package_type,
                            start_location,
                            unipro_mfg,
                            unipro_pid,
                            ara_vid,
                            ara_pid);

        if (success) {
            /* ...write it out and... */
            success = write_tftf_file(tftf_hdr, output_filename);
            if (success && map_flag) {
                /* ...and write out the map file */
                success = write_tftf_map_file(tftf_hdr, output_filename);
            }
        }

        if (success && verbose_flag) {
            print_tftf_file(tftf_hdr, output_filename);
        }

        free_tftf_header(tftf_hdr);
    }

    parse_tbl = free_argparse(parse_tbl);
    return program_status;
}

