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
 * @brief: This file contains the headers for FFFF utilities.
 *
 */

#ifndef _COMMON_FFFF_COMMON_H
#define _COMMON_FFFF_COMMON_H

#include "ffff.h"
#include <stddef.h>

/* Meta-information for an FFFF romimage */
struct ffff {
    uint32_t        blob_length;
    uint32_t        erase_block_length;
    ffff_header *   ffff_hdrs[2];
    uint8_t         blob[];
};


/**
 * @brief Debugging routine to display an element.
 *
 * @param title optional prefix for the line
 * @param element The element to display
 *
 * @returns If successful, returns a pointer to a blob with a
 */
void print_element(char * title, ffff_element_descriptor * element);

/**
 * @brief Macro to calculate the last address in an element.
 */
#define ELEMENT_END_ADDRESS(element_ptr) \
    ((element_ptr)->element_load_address + \
     (element_ptr)->element_expanded_length - 1)


/**
 * @brief Macro to calculate the number of elements in an FFFF header
 */
#define CALC_MAX_FFFF_ELEMENTS(header_size) \
		(((header_size) - \
		 (offsetof(ffff_header, elements) + FFFF_SENTINEL_SIZE)) / \
		 sizeof(ffff_element_descriptor))

#define FFFF_HEADER_SIZE_DEFAULT    FFFF_HEADER_SIZE_MAX


/* This contains the maximum number of elements in the header. */
extern uint32_t ffff_max_elements;


/**
 * Function Prototypes
 */


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
                       const uint32_t erase_block_length);


/**
 * @brief Free an FFFF romimage
 *
 * @param romimage Pointer to the FFFF romimage to free
 *
 * @returns NULL as a convenience to the caller, who can use
 *          "ptr = free_ffff(ptr)" to free it and safety the ptr in one
 *          statement.
 */
struct ffff *  free_ffff(struct ffff * romimage);


/**
 * @brief Set the timestamp field in the FFFF header to the current time.
 *
 * Set the timestamp field in the FFFF header to the current time as an ASCII
 * string of the format "YYYYMMDD HHMMSS", where the time is in UTC.
 *
 * @param ffff_header A pointer to the FFFF header in which it will set the
 * timestamp.
 *
 * @returns Nothing
 */
void ffff_set_timestamp(ffff_header * hdr);


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
int  ffff_element_collisions(const ffff_header * ffff_hdr,
                             const ffff_element_descriptor * element,
                             uint32_t * collisions,
                             const uint32_t  max_collisions);


/**
 * @brief Determine if 2 FFFF element tables match
 *
 * @param ffff_hdr_x Pointer to one FFFF header to compare
 * @param ffff_hdr_y Pointer to the otehr FFFF header
 *
 * @returns True if they match, false otherwise.
 */
bool  ffff_element_tables_match(const ffff_header * ffff_hdr_x,
                                const ffff_header * ffff_hdr_y);


/**
 * @brief Determine if 2 FFFF headers match
 *
 * @param ffff_hdr_x Pointer to one FFFF header to compare
 * @param ffff_hdr_y Pointer to the other FFFF header
 *
 * @returns True if they match, false otherwise.
 */
bool  ffff_headers_match(const ffff_header * ffff_hdr_x,
                         const ffff_header * ffff_hdr_y);

#endif /* !_COMMON_FFFF_COMMON_H */

