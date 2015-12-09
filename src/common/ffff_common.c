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
 * @brief: This file contains the common code for FFFF manipulation.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "util.h"
#include "ffff.h"
#include "ffff_common.h"

/**
 * This contains the maximum number of elements in the header.
 * NOTE: This initial value is a placeholder and will be overwritten in
 * "main()" using the CALC_MAX_FFFF_ELEMENTS macro to calculate the correct
 * value based on the header_size (which is unavailable until either command
 * line args have been parsed or an FFFF file read in).
 */
uint32_t ffff_max_elements =
        CALC_MAX_FFFF_ELEMENTS(FFFF_HEADER_SIZE_DEFAULT);


/**
 * @brief Debugging routine to display an element.
 *
 * @param title optional prefix for the line
 * @param element The element to display
 *
 * @returns If successful, returns a pointer to a blob with a
 */
void print_element(char * title, ffff_element_descriptor * element) {
    if (!title) {
        title = "";
    }
    printf("%s: @ %p [type %x class %x id %x len %x loc %x gen %x]\n",
           title, element, element->element_type,
           element->element_class, element->element_id,
           element->element_length, element->element_location,
           element->element_generation);
}


/**
 * @brief Allocate an FFFF romimage blob
 *
 * @param image_size The size in bytes of the FFFF romimage blob
 * @param header_length The size in bytes of the FFFF header
 * @param erase_block_length The size in bytes of the Flash erase block
 *
 * @returns If successful, returns a pointer to a blob with a
 *          minimally-initialized FFFF header at the beginning; NULL
 *          otherwise.
 */
struct ffff * new_ffff(const uint32_t image_size,
                       const uint32_t header_length,
                       const uint32_t erase_block_length) {
    struct ffff * romimage =  NULL;
    size_t length = sizeof(struct ffff) + image_size;
    uint32_t header_blob_length;

    romimage = malloc (length);
    if (!romimage) {
        fprintf(stderr, "ERROR: Can't allocate an FFFF romimage\n");
    } else {
        /* Initialization the allocated romimage */
        memset(romimage, 0, length);
        romimage->blob_length = image_size;
        romimage->erase_block_length = erase_block_length;
        romimage->ffff_hdrs[0] = (ffff_header *)&romimage->blob[0];
        header_blob_length = next_boundary(header_length, erase_block_length);
        if (header_blob_length != 0) {
            romimage->ffff_hdrs[1] =
                    (ffff_header *)&romimage->blob[header_blob_length];
        } else {
            /* The caller will fill this in later */
            romimage->ffff_hdrs[1] = NULL;
        }
    }

    return romimage;
}


/**
 * @brief Free an FFFF romimage
 *
 * @param romimage Pointer to the FFFF romimage to free
 *
 * @returns NULL as a convenience to the caller, who can use
 *          "ptr = free_ffff(ptr)" to free it and safety the ptr in one
 *          statement.
 */
struct ffff * free_ffff(struct ffff * romimage) {
    if (romimage) {
        free(romimage);
    }

    return NULL;
}


/**
 * @brief Set the timestamp field in the FFFF header to the current time.
 *
 * Set the timestamp field in the FFFF header to the current time as an ASCII
 * string of the format "YYYYMMDD HHMMSS", where the time is in UTC.
 *
 * @param ffff_hdr A pointer to the FFFF header in which it will set the
 * timestamp.
 *
 * @returns Nothing
 */
void ffff_set_timestamp(ffff_header * ffff_hdr) {
    time_t now_raw;
    struct tm * now;

    if (ffff_hdr) {
        /* Get the current time and convert it to UTC */
        now_raw = time(NULL);
        now = gmtime(&now_raw);

        /* Fill in the timestamp as "YYYYMMDD HHMMSS" */
        snprintf(ffff_hdr->build_timestamp,
                 sizeof(ffff_hdr->build_timestamp),
                 "%4d%02d%02d %02d%02d%02d",
                 now->tm_year + 1900,
                 now->tm_mon,
                 now->tm_mday,
                 now->tm_hour,
                 now->tm_min,
                 now->tm_sec);
    }
}


/**
 * @brief Determine if an FFFF element collides with any others
 *
 * @param ffff_hdr Pointer to the FFFF header to examine
 * @param element_index Index of the element in question
 * @param collisions Pointer to a table which will hold the indices of
 *        colliding elements.
 * @param max_collisions The number of entries in the collisions table.
 *
 * @returns The number of collisions.
 */
int ffff_element_collisions(const ffff_header * ffff_hdr,
                            const ffff_element_descriptor * element,
                            uint32_t * collisions,
                            const uint32_t  max_collisions) {
    int num_collisions = 0;

    if (!ffff_hdr || !collisions) {
        fprintf(stderr, "ERROR(ffff_element_collisions): invalid parameters\n");
    } else {
        uint32_t index;
        const ffff_element_descriptor * sweeper;

        for (index = 0, sweeper = ffff_hdr->elements;
             ((index < ffff_max_elements) &&
              (sweeper->element_type != FFFF_ELEMENT_END) &&
              (num_collisions < max_collisions));
             index++, sweeper++) {
            /**
             * Skip over ourself
             */
            if (sweeper != element) {
                if (regions_overlap(element->element_location,
                                    element->element_length,
                                    sweeper->element_location,
                                    sweeper->element_length)) {
                    collisions[num_collisions++] = index;
                }
            }
        }
    }

    return num_collisions;
}


/**
 * @brief Determine if 2 FFFF element tables match
 *
 * @param ffff_hdr_x Pointer to one FFFF header to compare
 * @param ffff_hdr_y Pointer to the other FFFF header
 *
 * @returns True if they match, false otherwise.
 */
bool  ffff_element_tables_match(const ffff_header * ffff_hdr_x,
                                const ffff_header * ffff_hdr_y) {
    const ffff_element_descriptor * x;
    const ffff_element_descriptor * y;
    const ffff_element_descriptor * limit;
    bool element_tables_match;

    if (!ffff_hdr_x || !ffff_hdr_x) {
        element_tables_match = false;
    } else {
        element_tables_match = true;
        x = &ffff_hdr_x->elements[0];
        y = &ffff_hdr_y->elements[0];
        limit = &ffff_hdr_x->elements[ffff_max_elements];

        for (; (x < limit); x++, y++) {
            if (memcmp(x, y, sizeof (*x)) != 0) {
                element_tables_match = false;
                break;
            }
            if (x->element_type == FFFF_ELEMENT_END) {
                break;
            }
        }
    }

    return element_tables_match;
}


/**
 * @brief Determine if 2 FFFF headers match
 *
 * @param ffff_hdr_x Pointer to one FFFF header to compare
 * @param ffff_hdr_y Pointer to the other FFFF header
 *
 * @returns True if they match, false otherwise.
 */
bool  ffff_headers_match(const ffff_header * ffff_hdr_x,
                         const ffff_header * ffff_hdr_y) {
    return (ffff_hdr_x && ffff_hdr_y &&
            memcmp(ffff_hdr_x, ffff_hdr_y, ffff_hdr_x->header_size) == 0);
}
