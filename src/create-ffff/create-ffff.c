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
 * @brief: This file contains the code for "create-ffff" a Linux command-line
 * app used to create a Flash Format For Firmware(FFFF) file, used by the
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



/* FFFF-specific parsing callbacks */
bool handle_element(const int option, const char * optarg,
                    struct optionx * optx);
bool handle_element_class(const int option, const char * optarg,
        struct optionx * optx);
bool handle_element_id(const int option, const char * optarg,
        struct optionx * optx);
bool handle_element_length(const int option, const char * optarg,
        struct optionx * optx);
bool handle_element_location(const int option, const char * optarg,
        struct optionx * optx);
bool handle_element_generation(const int option, const char * optarg,
        struct optionx * optx);


/* FFFF parsing args */
uint32_t flash_capacity;
uint32_t erase_block_size;
uint32_t image_length;
uint32_t generation;
char * output_filename;
uint32_t header_size;
char * name;
char * s2fw_filename;
char * s3fw_filename;
char * ims_filename;
char * cms_filename;
char * data_filename;
uint32_t element_class;
uint32_t element_id;
uint32_t element_generation;
uint32_t element_location;
uint32_t element_length;
int verbose_flag = false;
int map_flag = false;

static char * flash_capacity_names[] = { "flash-capacity", "fc", NULL };
static char * erase_block_size_names[] = { "erase-size", "ebs", NULL };
static char * image_length_names[] = { "image-length", "length", NULL };
static char * generation_names[] = { "generation", "gen", NULL };
static char * output_filename_names[] = { "out", NULL };
static char * header_size_names[] = { "header-size", NULL };
static char * name_names[] = { "name", NULL };
static char * s2fw_filename_names[] = { "stage-2-fw", "s2fw", NULL };
static char * s3fw_filename_names[] = { "stage-3-fw", "s3fw", NULL };
static char * ims_filename_names[] = { "ims", NULL };
static char * cms_filename_names[] = { "cms", NULL };
static char * data_filename_names[] = { "data", NULL };
static char * element_class_names[] = { "element-class", "eclass", NULL };
static char * element_id_names[] = { "element-id", "eid", NULL };
static char * element_generation_names[] =
    { "element-generation", "egen", NULL };
static char * element_location_names[] = { "element-location", "eloc", NULL };
static char * element_length_names[] = { "element-length", "elen", NULL };
static char * verbose_flag_names[] = { "verbose", NULL };
static char * map_flag_names[] = { "map", NULL };

static const char flash_capacity_help[] =
        "The capacity of the Flash drive, in bytes";
static const char erase_block_size_help[] =
        "The erase block granularity, in bytes";
static const char image_length_help[] =
        "The size of the image, in bytes";
static const char generation_help[] =
        "The header generation number (must be bigger than what is\n"
        "currently on the Flash)";
static const char output_filename_help[] =
        "Specifies the output FFFF file";
static const char header_size_help[] =
        "The size of the generated FFFF header, in bytes (4096)";
static const char name_help[] = "Flash image name";
static const char s2fw_filename_help[] = "Stage 2 Firmware file";
static const char s3fw_filename_help[] = "Stage 3 Firmware file";
static const char ims_filename_help[] = "IMS certificate file";
static const char cms_filename_help[] = "CMS certificate file";
static const char data_filename_help[] = "Generic data file";
static const char element_class_help[] = "The element's class number";
static const char element_id_help[] = "The element's ID number";
static const char element_location_help[] =
        "The element's absolute location in Flash (must be a multiple\n"
        "of --erase-size)";
static const char element_length_help[] =
        "(Optional) The element's length. If ommitted, the length is\n"
        "extracted from the file";
static const char element_generation_help[] =
        "The element's generation number";
static const char verbose_flag_help[] =
        "Display the FFFF header and a synopsis of each FFFF section";
static const char map_flag_help[] =
        "Create a map file of the FFFF headers and each FFFF sections";


/* Parsing table */
static struct optionx parse_table[] = {
    { 'f', flash_capacity_names, "num", &flash_capacity, 0,
      DEFAULT_VAL, &store_hex, 0, flash_capacity_help },
    { 'e', erase_block_size_names, "num", &erase_block_size, 0,
      DEFAULT_VAL,    &store_hex, 0, erase_block_size_help },
    { 'l', image_length_names, "num", &image_length, 0,
      DEFAULT_VAL,    &store_hex, 0, image_length_help },
    { 'g', generation_names, "num", &generation, 0,
      DEFAULT_VAL,    &store_hex, 0, generation_help },
    { 'o', output_filename_names, "file",  &output_filename, 0,
      0, &store_str, 0, output_filename_help },
    { 'h', header_size_names, "num", &header_size, FFFF_HEADER_SIZE,
      DEFAULT_VAL,    &store_hex, 0, header_size_help },
    { 'n', name_names, "text", &name, 0,
       0, &store_str, 0, name_help },

    /* <element_type>: */
    { '2', s2fw_filename_names, "file", &s2fw_filename, 0,
      0, &handle_element, 0, s2fw_filename_help },
    { '3', s3fw_filename_names, "file", &s3fw_filename, 0,
      0, &handle_element, 0, s3fw_filename_help },
    { 'i', ims_filename_names, "file", &ims_filename, 0,
      0, &handle_element, 0, ims_filename_help },
    { 'c', cms_filename_names, "file", &cms_filename, 0,
      0, &handle_element, 0, cms_filename_help },
    { 'd', data_filename_names, "file", &data_filename, 0,
      0, &handle_element, 0, data_filename_help },

    /* <element_option>: */
    { 'C', element_class_names, "num", &element_class, 0,
      DEFAULT_VAL, &handle_element_class, 0, element_class_help },
    { 'I', element_id_names, "num", &element_id, 0,
      DEFAULT_VAL, &handle_element_id, 0, element_id_help },
    { 'L', element_length_names, "num", &element_length, 0,
      DEFAULT_VAL, &handle_element_length, 0, element_length_help },
    { 'O', element_location_names, "num", &element_location, 0,
      DEFAULT_VAL, &handle_element_location, 0, element_location_help },
    { 'G', element_generation_names, "num", &element_generation, 0,
      DEFAULT_VAL, &handle_element_generation, 0, element_generation_help },

    { 'v', verbose_flag_names, NULL, &verbose_flag, 0,
      STORE_TRUE, &store_flag, 0, verbose_flag_help },
    { 'm', map_flag_names, NULL,  &map_flag, 0,
      STORE_TRUE, &store_flag, 0, map_flag_help },
    { 0, NULL, NULL, NULL, 0, 0, NULL, 0 , NULL}
};


/* The 1-char tags for all args */
static char * all_args = "f:e:l:g:o:h:n:2:3:i:c:d:C:I:G:O:L:vm";

/**
 * The 1-char tags for each of the element-related args
 * (close_element_if_needed uses this to filter)
 */
static char * element_args = "23icdCIGOL";


static char * epilog =
    "NOTE: elements are specified as [<element_type> <file>  <element_option>]...\n"
    "   <element_type> ::= [--s2f | --s3f | --ims | --cms | --data]\n"
    "   <element_option> ::= {--eclass} {--eid} {--eloc} {--elen}";


/**
 * @brief Callback to load an element.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_element(const int option, const char * optarg,
                           struct optionx * optx) {
    bool success;
    bool known_arg = true;

    switch (option) {
    case '2':   /* s2fw */
        success =
            element_cache_entry_open(FFFF_ELEMENT_STAGE_2_FW, optarg) == 0;
        break;

    case '3':   /* s3fw */
        success =
            element_cache_entry_open(FFFF_ELEMENT_STAGE_3_FW, optarg) == 0;
        break;

    case 'i':   /* IMS certificate */
        /* TODO: We may want to validate the signature we fetch */
        success =
            element_cache_entry_open(FFFF_ELEMENT_IMS_CERT, optarg) == 0;
        break;

    case 'c':   /* CMS certificate */
        /* TODO: We may want to validate the certificate we fetch */
        success =
            element_cache_entry_open(FFFF_ELEMENT_CMS_CERT, optarg) == 0;
        break;

    case 'd':   /* data */
        success = element_cache_entry_open(FFFF_ELEMENT_DATA, optarg) == 0;
        break;

    default:
        /* We should never get here */
        success = false;
        known_arg = false;
    }

    if (!success) {
        if (known_arg) {
            fprintf(stderr, "ERROR: --%s %s failed\n", (optx->long_names)[0], optarg);
        } else {
            fprintf(stderr,
                    "ERROR: unknown section type '%c'\n", option);
        }
    }
    return success;
}


/**
 * @brief Callback to load the element class into the current element cache.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_element_class(const int option, const char * optarg,
                          struct optionx * optx) {
    uint32_t num;

    return get_num(optarg, (optx->long_names)[0], &num) &&
            element_cache_entry_set_class(num);
}


/**
 * @brief Callback to load the element id into the current element cache.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_element_id(const int option, const char * optarg,
                          struct optionx * optx) {
    uint32_t num;

    return get_num(optarg, (optx->long_names)[0], &num) &&
            element_cache_entry_set_id(num);
}


/**
 * @brief Callback to load the element length into the current element cache.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_element_length(const int option, const char * optarg,
                          struct optionx * optx) {
    uint32_t num;

    return get_num(optarg, (optx->long_names)[0], &num) &&
            element_cache_entry_set_length(num);
}


/**
 * @brief Callback to load the element location into the current element cache.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_element_location(const int option, const char * optarg,
                          struct optionx * optx) {
    uint32_t num;

    return get_num(optarg, (optx->long_names)[0], &num) &&
            element_cache_entry_set_location(num);
}


/**
 * @brief Callback to load the element generation into the current element cache.
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 * @param optarg The string from the argument parser
 * @param optx A pointer to the option descriptor
 *
 * @returns Returns true on success, false on failure
 */
bool handle_element_generation(const int option, const char * optarg,
                          struct optionx * optx) {
    uint32_t num;

    return get_num(optarg, (optx->long_names)[0], &num) &&
            element_cache_entry_set_generation(num);
}


/**
 * @brief Callback to close the element window when we see a non-section arg
 *
 * @param option The option character (may be used to disambiguate a
 *        common function)
 *
 * @returns Returns nothing
 */
void close_element_if_needed(const int option) {
    if (index(element_args, option) == 0) {
        element_cache_entry_close();
    }
}


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
    struct ffff * romimage = NULL;
    struct argparse * parse_tbl = NULL;
    int program_status = PROGRAM_SUCCESS;

    /* Parse the command line arguments */
    parse_tbl = new_argparse(parse_table, argv[0], NULL, epilog, NULL,
                             close_element_if_needed);
    if (parse_tbl) {
        success =  parse_args(argc, argv, all_args, parse_tbl);
    } else {
        success = false;
    }

    if (success) {
        /* Make sure we close off any under-construction section */
        element_cache_entry_close();

        /* Validate that we have the needed args */
        if (!output_filename) {
            fprintf(stderr, "Error: no output file specified\n");
            success = false;
        }
        else if (element_cache_entry_count() == 0) {
            fprintf(stderr,
                    "%s: missing input elements: s2f, s3f, ims, cms, data\n",
                    argv[0]);
            success = false;
        }

        /* Verify that the element locations make sense */
        if (!element_cache_validate_locations(header_size,
                                              erase_block_size,
                                              image_length)) {
            success = false;
        }
    }

    if (!success) {
         usage(parse_tbl);
        program_status = PROGRAM_ERRORS;
    } else {
        /* Calculate the number of sections in this ffff header */
        /* TODO: Use macro (TBD) in tftf.h to calculate max_sections */
        ffff_max_elements = CALC_MAX_FFFF_ELEMENTS(header_size);

        /* Create and initialize the TFTF blob... */
        romimage = new_ffff_romimage(name,
                                     flash_capacity,
                                     erase_block_size,
                                     image_length,
                                     generation,
                                     header_size);
        if (success) {
            /* ...write it out and... */
            success = write_ffff_file(romimage, output_filename);
            if (success && map_flag) {
                /* ...and write out the map file */
                success = write_ffff_map_file(romimage, output_filename);
            }
        }

        if (success && verbose_flag) {
            print_ffff_file(romimage, output_filename);
        }

        free_ffff(romimage);
    }

    parse_tbl = free_argparse(parse_tbl);
    return program_status;
}

