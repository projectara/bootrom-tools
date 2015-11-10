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
 * @brief: This file contains the headers for TFTF utilities.
 *
 */

#ifndef _COMMON_TFTF_COMMON_H
#define _COMMON_TFTF_COMMON_H

#include "tftf.h"

static const char tftf_sentinel[] = TFTF_SENTINEL_VALUE;

/* TFTF Parser defaults */
#define DFLT_START          0
#define DFLT_UNIPRO_MID     0
#define DFLT_UNIPRO_PID     0
#define DFLT_ARA_VID        0
#define DFLT_ARA_PID        0
#define DFLT_ARA_BOOT_STAGE 2
#define DFLT_SECT_ID        0
#define DFLT_SECT_CLASS     0
#define DFLT_SECT_LOAD      ((uint32_t)-1)



/**
 * @brief Macro to calculate the last address in a section.
 */
#define SECTION_END_ADDRESS(section_ptr) \
    ((section_ptr)->section_load_address + \
     (section_ptr)->section_expanded_length - 1)


#if 0
typedef struct
{
    const char *    filename;
    int             type;
    uint32_t        offset;     /* 0 means no offset specified */
} section_info;
#endif


/**
 * If compress is true, compress the data while copying; if false, just
 * perform a raw copy. (Ignored if COMPRESSED_SUPPORT is not defined.)
 */
extern bool compress;

/* if verbose is true, print out a summary of the TFTF header on success. */
extern bool verbose;


/**
 * Function Prototypes
 */


/**
 * @brief Allocate a TFTF blob
 *
 * @returns If successful, returns a pointer to a blob with a
 *          minimally-initialized TFTF header at the beginning; NULL
 *          otherwise.
 */
tftf_header * new_tftf_blob(uint32_t header_size, uint32_t payload_size);


/**
 * @brief Free a TFTF header/blob
 *
 * @param header Pointer to the TFTF header to free
 *
 * @returns NULL as a convenience to the caller, who can use
 *          "ptr = free_tftf_header(ptr)" to free it and safety the ptr in one
 *          statement.
 */
tftf_header *  free_tftf_header(tftf_header * hdr);


/**
 * @brief Determine the total payload size
 *
 * @param tftf_hdr Pointer to the TFTF header to examine
 *
 * @returns The number of payload bytes.
 */
size_t  tftf_payload_size(tftf_header * tftf_hdr);


/**
 * @brief Set the timestamp field in the TFTF header to the current time.
 *
 * Set the timestamp field in the TFTF header to the current time as an ASCII
 * string of the format "YYYYMMDD HHMMSS", where the time is in UTC.
 *
 * @param tftf_header A pointer to the TFTF header in which it will set the
 * timestamp.
 *
 * @returns Nothing
 */
void set_timestamp(tftf_header * hdr);


/**
 * @brief Determine if a TFTF section collides with any others
 *
 * @param tftf_hdr Pointer to the TFTF header to examine
 * @param section_index Index of the section in question
 * @param collisions Pointer to a table which will hold the indices of
 *        colliding sections.
 * @param max_collisions The number of entries in the collisions table.
 *
 * @returns The number of collisions.
 */
int  tftf_section_collisions(tftf_header * tftf_hdr,
                                tftf_section_descriptor * section,
                                uint32_t * collisions,
                                uint32_t  max_collisions);

#endif /* !_COMMON_TFTF_COMMON_H */

