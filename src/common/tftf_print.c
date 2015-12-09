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
 * @brief: This file contains the includes for displaying a TFTF file
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
#include "tftf.h"
#include "tftf_common.h"
#include "tftf_print.h"



/**
 * @brief Convert a TFTF section type into a human-readable string
 *
 * @param type the TFTF section type
 *
 * @returns A string
 */
const char * tftf_section_type_name(const uint8_t type) {
    char * name = "?";

    switch (type) {
    case TFTF_SECTION_RAW_CODE:
        name = "code";
        break;
    case TFTF_SECTION_RAW_DATA:
        name = "data";
        break;
    case TFTF_SECTION_COMPRESSED_CODE:
        name = "compressed code";
        break;
    case TFTF_SECTION_COMPRESSED_DATA:
        name = "compressed data";
        break;
    case TFTF_SECTION_MANIFEST:
        name = "manifest";
        break;
    case TFTF_SECTION_SIGNATURE:
        name = "signature";
        break;
    case TFTF_SECTION_CERTIFICATE:
        name = "certificate";
        break;
    case TFTF_SECTION_END:
        name = "end of sections";
        break;
    }
    return name;
}


/**
 * @brief Print out the data/payload associated with a TFTF signature
 *
 * @param header Pointer to the TFTF signature data to display
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
static void print_tftf_signature(const tftf_signature * sig_block,
                                 const char * indent) {
    if (sig_block) {
        char data_indent[256];

        /* Format the indent for the binary data */
        if (!indent) {
            indent = "";
        }
        snprintf(data_indent, sizeof(data_indent), "%s       ", indent);

        printf("%s  Length:    %08x\n", indent, sig_block->length);
        printf("%s  Sig. type: %d (%s)\n", indent, sig_block->type,
               tftf_section_type_name(sig_block->type));
        printf("%s  Key name:\n", indent);
        printf("%s      '%4s'\n", indent, sig_block->key_name);
        printf("%s  Signature:\n", indent);
        display_binary_data((uint8_t *)sig_block->signature, sig_block->length, true,
                            data_indent);
    }
}


/**
 * @brief Print out the data/payload associated with the FTF header section table
 *
 * @param header Pointer to the TFTF header  section descriptor to display
 * @param title_string Optional title string
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
static void print_tftf_section_data(const tftf_header * tftf_hdr,
                                    const char * title_string,
                                    const char * indent) {

    if (tftf_hdr) {
        int index;
        const tftf_section_descriptor * section = tftf_hdr->sections;
        uint8_t * pdata = NULL;
        char indent_section_hdr[256];
        char indent_section_contents[256];

        /* Format the indent for the binary data */
        if (!indent) {
            indent = "";
        }
        snprintf(indent_section_hdr, sizeof(indent_section_hdr),
                 "%s  ", indent);
        snprintf(indent_section_contents, sizeof(indent_section_contents),
                 "%s    ", indent);

        /* 1. Print the title line */
        if (title_string) {
            printf("\n%sTFTF section contents for %s (%u bytes)\n",
                   indent, title_string,
                   (uint32_t)tftf_payload_size(tftf_hdr));
        } else {
            printf("%sTFTF contents (%u bytes)\n",
                   indent, (uint32_t)tftf_payload_size(tftf_hdr));
        }

        /* 2. Print the associated data blobs */
        for (index = 0, pdata = ((uint8_t *)tftf_hdr) + tftf_hdr->header_size;
             ((index < tftf_max_sections) &&
              (section->section_type != TFTF_SECTION_END));
             pdata += section->section_length, index++, section++) {
            /* Print a generic section header */
            printf("%s  section [%d] (%d bytes): %s\n",
                   indent, index, section->section_length,
                   tftf_section_type_name(section->section_type));

            /* Print the section data proper */
            switch (section->section_type) {
            case TFTF_SECTION_SIGNATURE:
                print_tftf_signature((tftf_signature *)pdata, indent);
                break;

            /**
             * The following section types either have no structure (e.g.,
             * code, data), or the structure is currently unkown (e.g.
             * manifest) Just show them as raw binary data for now.
             */
            case TFTF_SECTION_RAW_CODE:
            case TFTF_SECTION_RAW_DATA:
            case TFTF_SECTION_COMPRESSED_CODE:
            case TFTF_SECTION_COMPRESSED_DATA:
            case TFTF_SECTION_MANIFEST:
            case TFTF_SECTION_CERTIFICATE:
                display_binary_data(pdata, section->section_length, false,
                                    indent_section_contents);
                printf ("\n");
                break;

            /**
             * The following section types  have no data.
             */
            case TFTF_SECTION_END:
                break;

            /* A type with which we're unfamiliar. */
            default:
                fprintf(stderr, "ERROR: Section [%d] has an unknown type: 0x%02x\n",
                        index, (uint32_t)section->section_type);
                break;
            }
        }
    }
}


/**
 * @brief Print out a TFTF header section descriptor
 *
 * @param header Pointer to the TFTF header  section descriptor to display
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
static void print_tftf_section_table(const tftf_header * tftf_hdr,
                                     const char * indent) {
    int index;
    const tftf_section_descriptor * section = tftf_hdr->sections;
    uint32_t * collisions = NULL;
    size_t max_collisions;
    size_t num_collisions;
    int collision;


    if (!tftf_hdr) {
        fprintf(stderr, "ERROR: No TFTF section table to print\n");
        return;
    }


    /* Allocate our collision table based on the size of the header */
    max_collisions = CALC_MAX_TFTF_SECTIONS(tftf_hdr->header_size);
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
    printf("%s  Section Table (all values in hex):\n", indent);
    printf("%s     Type Class  ID       Length   Load     Exp.Len\n", indent);

    /* Print out the occupied entries in the section table */
    for (index = 0; index < tftf_max_sections; index++, section++) {
        /* print the section header info */
        printf("%s  %2d %02x   %06x %08x %08x %08x %08x (%s)\n",
               indent,
               index,
               (uint32_t)section->section_type,
               (uint32_t)section->section_class,
               section->section_id,
               section->section_length,
               section->section_load_address,
               section->section_expanded_length,
               tftf_section_type_name(section->section_type));

        /* Note any collisions on a separate line */
        num_collisions = tftf_section_collisions(tftf_hdr, section,
                                                 collisions, num_collisions);
        if (num_collisions > 0) {
            printf("%s     (Collides with section%s:",
                   indent, (num_collisions > 1)? "s" : "");
            for (collision = 0; collision < num_collisions; collision++) {
                printf (" %d", collisions[collision]);
            }
            printf (")\n");
        }

        /* Done? */
        if (section->section_type == TFTF_SECTION_END) {
            break;
        }
    }

    /* Print out the unused sections */
    if (index == (tftf_max_sections - 1)) {
        /* 1 unused section */
        printf("%s  %2d (unused)\n", indent, index);
    } else if (index == (tftf_max_sections - 2)) {
        /* 2 unused sections */
        printf("%s  %2d (unused)\n", indent, index);
        printf("%s  %2d (unused)\n", indent, tftf_max_sections - 1);
    } else {
        /* Many unused, use elipsis */
        printf("%s  %2d (unused)\n", indent, index);
        printf("%s   :    :\n", indent);
        printf("%s  %2d (unused)\n", indent, tftf_max_sections - 1);
    }

    /* Dispose of the collisions table */
    if (collisions) {
        free(collisions);
    }
}


/**
 * @brief Print out a TFTF header
 *
 * @param header Pointer to the TFTF header to display
 * @param title_string Optional title string
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
static void print_tftf_header(const tftf_header * tftf_hdr,
                              const char * title_string,
                              const char * indent) {
    if (tftf_hdr) {
        const char * ptr = tftf_hdr->sentinel_value;
        uint32_t * sentinel = (uint32_t *)tftf_hdr->sentinel_value;
        int index;

        /* Ensure the indent prints nicely */
        if (!indent) {
            indent = "";
        }

        /* 1. Print the title line */
        if (title_string) {
            printf("%sTFTF Header for %s (%u bytes)\n",
                   indent, title_string,
                   (uint32_t)tftf_payload_size(tftf_hdr));
        } else {
            printf("%sTFTF Header (%u bytes)\n",
                   indent, (uint32_t)tftf_payload_size(tftf_hdr));
        }


        /* 2. Print the header */
        /* Print out the fixed part of the header */
        printf("%s  Sentinel:         '%c%c%c%c' (%08x)\n",
                indent,
                isprint(ptr[0])? ptr[0] : '-',
                isprint(ptr[1])? ptr[1] : '-',
                isprint(ptr[2])? ptr[2] : '-',
                isprint(ptr[3])? ptr[3] : '-',
                *sentinel);
        printf("%s  Header size:       %08x\n",
               indent, tftf_hdr->header_size);
        printf("%s  Timestamp:        '%s'\n",
               indent,  tftf_hdr->build_timestamp);
        printf("%s  Fw. pkg name:     '%s'\n",
               indent,  tftf_hdr->firmware_package_name);
        printf("%s  Package type:      %08x\n",
               indent, tftf_hdr->package_type);
        printf("%s  Start location:    %08x\n",
               indent, tftf_hdr->start_location);
        printf("%s  Unipro mfg ID:     %08x\n",
               indent, tftf_hdr->unipro_mid);
        printf("%s  Unipro product ID: %08x\n",
               indent, tftf_hdr->unipro_pid);
        printf("%s  Ara vendor ID:     %08x\n",
               indent, tftf_hdr->ara_vid);
        printf("%s  Ara product ID:    %08x\n",
               indent, tftf_hdr->ara_pid);
        for (index = 0; index < TFTF_NUM_RESERVED; index++) {
            printf("%s    Reserved [%d]:    %08x\n",
                   indent, index, tftf_hdr->reserved[index]);
        }

        /* Print out the variable part of the header */
        print_tftf_section_table(tftf_hdr, indent);
    }
}


/**
 * @brief Print out a TFTF blob
 *
 * @param header Pointer to the TFTF header to display
 * @param title_string Optional title string
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
void print_tftf(const tftf_header * tftf_hdr,
                const char * title_string,
                const char * indent) {
    if (tftf_hdr) {
        /* Ensure the indent prints nicely */
        if (!indent) {
            indent = "";
        }
        print_tftf_header(tftf_hdr, title_string, indent);
        print_tftf_section_data(tftf_hdr, title_string, indent);
    }
}


/**
 * @brief Print out a TFTF file
 *
 * Prints out an in-memory TFTF blob, associated with a file.
 *
 * @param header Pointer to the TFTF header to display
 * @param title_string Optional title string
 *
 * @returns Nothing
 */
void print_tftf_file(const tftf_header * tftf_hdr, const char * filename) {
    if ((tftf_hdr) &&(filename)) {
        print_tftf_header(tftf_hdr, filename, NULL);
        print_tftf_section_data(tftf_hdr, filename, NULL);
    } else {
        fprintf(stderr, "ERROR: Missing TFTF header or filename\n");
    }
}

