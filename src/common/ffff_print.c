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
 * @brief: This file contains the includes for displaying a FFFF file
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "ffff.h"
#include "ffff_common.h"
#include "ffff_print.h"
#include "tftf.h"
#include "tftf_print.h"



/**
 * @brief Convert a FFFF element type into a human-readable string
 *
 * @param type the FFFF element type
 *
 * @returns A string
 */
const char * ffff_element_type_name(const uint8_t type) {
    char * name = "?";

    switch (type) {
    case FFFF_ELEMENT_STAGE_2_FW:
        name = "2fw";
        break;
    case FFFF_ELEMENT_STAGE_3_FW:
        name = "3fw";
        break;
    case FFFF_ELEMENT_IMS_CERT:
        name = "ims";
        break;
    case FFFF_ELEMENT_CMS_CERT:
        name = "cms";
        break;
    case FFFF_ELEMENT_DATA:
        name = "data";
        break;
    case FFFF_ELEMENT_END:
        name = "end";
        break;
    }
    return name;
}


/**
 * @brief Print out the data/payload associated with the FTF header element table
 *
 * @param romimage Pointer to the FFFF to display
 * @param header Pointer to the FFFF header  element descriptor to display
 * @param title_string Optional title string
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
static void print_ffff_element_data(const struct ffff *romimage,
                                    const ffff_header * ffff_hdr,
                                    const char * title_string,
                                    const char * indent) {
    int index;
    const ffff_element_descriptor * element;
    const tftf_header * tftf_hdr;
    bool data_is_tftf;
    char indent_buf[256];
    char indent_data_buf[256];

    if (ffff_hdr) {
        int index;
        const ffff_element_descriptor * element = ffff_hdr->elements;
        uint8_t * pdata = NULL;

        /* Format the indent for the binary data */
        if (!indent) {
            indent = "";
        }
        snprintf(indent_buf, sizeof(indent_buf),
                 "%s  ", indent);
        snprintf(indent_data_buf, sizeof(indent_buf),
                 "%s    ", indent);

        /* 1. Print the title line */
        if (title_string) {
            printf("%s%s:\n", indent, title_string);
        } else {
            printf("%sFFFF contents:\n", indent);
        }

        /* 2. Print the associated data blobs */
        for (index = 0, element = ffff_hdr->elements;
             ((index < ffff_max_elements) &&
              (element->element_type != FFFF_ELEMENT_END));
             index++, element++) {
            /* Print a generic element header */
             printf("%selement [%d] (%s) (%d bytes):\n",
                    indent_buf, index,
                    ffff_element_type_name(element->element_type),
                    element->element_length);

             /* Print the element data proper */
             tftf_hdr = (const tftf_header *)&romimage->blob[element->element_location];
             if (sniff_tftf_header(tftf_hdr)) {
                 /* We know how to print TFTF elements */
                 print_tftf(tftf_hdr,
                            ffff_element_type_name(element->element_type),
                            indent_data_buf);
             } else {
                 /* No idea what format it has, use a data dump */
                 display_binary_data(&romimage->blob[element->element_location],
                                     element->element_length,
                                     false,
                                     indent_data_buf);
             }
        }
    }
    printf("\n");
}


/**
 * @brief Print out a FFFF header element descriptor
 *
 * @param header Pointer to the FFFF header  element descriptor to display
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
static void print_ffff_element_table(const ffff_header * ffff_hdr,
                                     const char * indent) {
    int index;
    const ffff_element_descriptor * element = ffff_hdr->elements;
    uint32_t * collisions = NULL;
    size_t max_collisions;
    size_t num_collisions;
    int collision;


    if (ffff_hdr) {
        /* Allocate our collision table based on the size of the header */
        max_collisions = CALC_MAX_FFFF_ELEMENTS(ffff_hdr->header_size);
        collisions = (uint32_t *)malloc(max_collisions * sizeof(uint32_t));
        if (!collisions) {
            fprintf(stderr, "ERROR: Can't allocate collision table\n");
            return;
        }

        /* Ensure the indent prints nicely */
        if (!indent) {
            indent = "";
        }

        /* Column headers */
        printf("%s  Element Table (all values in hex):\n", indent);
        printf("%s     Type Class  ID       Length   Location Generation\n",
               indent);

        /* Print out the occupied entries in the element table */
        for (index = 0; index < ffff_max_elements; index++, element++) {
            /* print the element header info */
            printf("%s  %2d %02x   %06x %08x %08x %08x %08x (%s)\n",
                   indent,
                   index,
                   (uint32_t)element->element_type,
                   (uint32_t)element->element_class,
                   element->element_id,
                   element->element_length,
                   element->element_location,
                   element->element_generation,
                   ffff_element_type_name(element->element_type));

            /* Note any collisions on a separate line */
            num_collisions = ffff_element_collisions(ffff_hdr, element,
                                                     collisions,
                                                     num_collisions);
            if (num_collisions > 0) {
                printf("     Collides with element(s):");
                for (collision = 0; collision < num_collisions; collision++) {
                    printf (" %d", collisions[collision]);
                }
                printf ("\n");
            }

            /* Done? */
            if (element->element_type == FFFF_ELEMENT_END) {
                break;
            }
        }

       /* Print out the unused elements */
        if (index == (ffff_max_elements - 1)) {
            /* 1 unused element */
            printf("%s  %2d (unused)\n", indent, index);
        } else if (index == (ffff_max_elements - 2)) {
            /* 2 unused elements */
            printf("%s  %2d (unused)\n", indent, index);
            printf("%s  %2d (unused)\n", indent, ffff_max_elements - 1);
        } else {
            /* Many unused, use elipsis */
            printf("%s  %2d (unused)\n", indent, index);
            printf("%s   :    :\n", indent);
            printf("%s  %2d (unused)\n", indent, ffff_max_elements - 1);
        }

        /* Dispose of the collisions table */
        if (collisions) {
            free(collisions);
    }
    }
}


/**
 * @brief Print out a FFFF header
 *
 * @param header Pointer to the FFFF header to display
 * @param title_string Optional title string
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
static void print_ffff_header(const ffff_header * ffff_hdr,
                              const char * title_string,
                              const char * indent) {
    if (ffff_hdr) {
        const char * ptr = ffff_hdr->sentinel_value;
        uint32_t * sentinel = (uint32_t *)ffff_hdr->sentinel_value;
        char sentinel_buf[FFFF_SENTINEL_SIZE + 1];
        int index;


        /* Ensure the indent prints nicely */
        if (!indent) {
            indent = "";
        }

        /* 1. Print the title line */
        if (title_string) {
            printf("%s%s:\n", indent, title_string);
        }


        /* 2. Print the header */
        /* Print out the fixed part of the header */
        safer_strncpy(sentinel_buf, sizeof(sentinel_buf),
                      ffff_hdr->sentinel_value, FFFF_SENTINEL_SIZE);
        printf("%s  Sentinel:         '%s'\n", indent, sentinel_buf);
        printf("%s  Timestamp:        '%s'\n",
               indent,  ffff_hdr->build_timestamp);
        printf("%s  Image_name:       '%s'\n",
               indent,  ffff_hdr->flash_image_name);
        printf("%s  flash_capacity:    %08x\n",
               indent, ffff_hdr->flash_capacity);
        printf("%s  erase_block_size:  %08x\n",
               indent, ffff_hdr->erase_block_size);
        printf("%s  Header_size:       %08x\n",
               indent, ffff_hdr->header_size);
        printf("%s  flash_image_length:%08x\n",
               indent, ffff_hdr->flash_image_length);
        for (index = 0; index < FFFF_RESERVED; index++) {
            printf("%s    Reserved [%d]:    %08x\n",
                   indent, index, ffff_hdr->reserved[index]);
        }

        /* Print out the variable part of the header */
        print_ffff_element_table(ffff_hdr, indent);

        /* Print out the tail sentinel */
        safer_strncpy(sentinel_buf, sizeof(sentinel_buf),
                      ffff_hdr->trailing_sentinel_value, FFFF_SENTINEL_SIZE);
        printf("%s  Tail sentinel:    '%s'\n", indent, sentinel_buf);
        printf("\n");
    }
}


/**
 * @brief Print out an FFFF romimage Typically called from "create_ffff".
 *
 * @param romimage Pointer to the FFFF to display
 * @param filename Optional title string
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
void print_ffff(const struct ffff *romimage,
                const char * filename,
                const char * indent) {
    char title_buf[256];

    if (romimage) {
        /* Ensure the indent prints nicely */
        if (!indent) {
            indent = "";
        }

        /* Print the header */
        if (filename) {
            printf("%sFFFF file %s (%u bytes):\n",
                   indent, filename,
                   romimage->ffff_hdrs[0]->flash_image_length);
        } else {
            printf("%s (%u bytes):\n",
                   indent, romimage->ffff_hdrs[0]->flash_image_length);
        }
        if (ffff_headers_match(romimage->ffff_hdrs[0],
                               romimage->ffff_hdrs[1])) {
            print_ffff_header(romimage->ffff_hdrs[0], "Combined FFFF headers",
                              indent);
            print_ffff_element_data(romimage, romimage->ffff_hdrs[0],
                                    "Combined FFFF header contents",
                                    indent);
        } else {
            print_ffff_header(romimage->ffff_hdrs[0], "FFFF header[0]",
                              indent);
            print_ffff_element_data(romimage, romimage->ffff_hdrs[0],
                                    "FFFF header[0] contents", indent);

            print_ffff_header(romimage->ffff_hdrs[1], "FFFF header[1]",
                              indent);
            print_ffff_element_data(romimage, romimage->ffff_hdrs[1],
                                    "FFFF header[0] contents",indent);
        }
    }
}


/**
 * @brief Print out an FFFF file
 *
 * Prints out an in-memory FFFF blob, associated with a file. Typically
 * called from "create_ffff".
 *
 * @param romimage Pointer to the FFFF to display
 * @param filename Name of the FFFF file
 *
 * @returns Nothing
 */
void print_ffff_file(const struct ffff *romimage, const char * filename) {
    if ((romimage) &&(filename)) {
        print_ffff(romimage, filename, NULL);
    } else {
        fprintf(stderr, "ERROR: Missing FFFF header or filename\n");
    }
}

