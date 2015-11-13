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
 * @brief: This file contains the common code for TFTF manipulation.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include <time.h>
#include "tftf.h"
#include "tftf_common.h"

/* if verbose is true, print out a summary of the TFTF header on success. */
int verbose_flag = false;

/* This contains the maximum number of sections in the header. */
uint32_t tftf_max_sections = TFTF_MAX_SECTIONS;


/**
 * @brief Allocate a TFTF blob
 *
 * @param header_size The size of the header
 * @param payload_size The size of all the payload sections
 *
 * @returns If successful, returns a pointer to a blob with a
 *          minimally-initialized TFTF header at the beginning; NULL
 *          otherwise.
 */
tftf_header * new_tftf_blob(uint32_t header_size, uint32_t payload_size) {
    tftf_header * tftf_hdr = NULL;
    size_t length = header_size + payload_size;

    tftf_hdr = malloc (length);
    if (!tftf_hdr) {
        fprintf(stderr, "ERROR: Can't allocate a TFTF\n");
    } else {
        /* Perform minimal initialization on the allocated buffer */
        memset(tftf_hdr, 0, length);
        memcpy(tftf_hdr->sentinel_value, tftf_sentinel, sizeof (tftf_sentinel));
        tftf_hdr->header_size = header_size;
    }

    return tftf_hdr;
}


/**
 * @brief Free a TFTF header/blob
 *
 * @param tftf_hdr Pointer to the TFTF header to free
 *
 * @returns NULL as a convenience to the caller, who can use
 *          "ptr = free_tftf_header(ptr)" to free it and safety the ptr in one
 *          statement.
 */
tftf_header *  free_tftf_header(tftf_header * tftf_hdr) {
    if (tftf_hdr) {
        free(tftf_hdr);
    }

    return NULL;
}


/**
 * @brief Determine the total payload size
 *
 * @param tftf_hdr Pointer to the TFTF header to examine
 *
 * @returns The number of payload bytes.
 */
size_t  tftf_payload_size(const tftf_header * tftf_hdr) {
    size_t size = 0;

    if (tftf_hdr) {
        const tftf_section_descriptor * section;
        uint8_t * hdr_end = ((uint8_t *)tftf_hdr) + tftf_hdr->header_size;

        for (section = tftf_hdr->sections;
             (section < (tftf_section_descriptor *)hdr_end) &&
             (section->section_type != TFTF_SECTION_END);
             section++) {
            size += section->section_length;
        }
    }

    return size;
}


/**
 * @brief Set the timestamp field in the TFTF header to the current time.
 *
 * Set the timestamp field in the TFTF header to the current time as an ASCII
 * string of the format "YYYYMMDD HHMMSS", where the time is in UTC.
 *
 * @param tftf_hdr A pointer to the TFTF header in which it will set the
 * timestamp.
 *
 * @returns Nothing
 */
void set_timestamp(tftf_header * tftf_hdr) {
    time_t now_raw;
    struct tm * now;

    if (tftf_hdr) {
        /* Get the current time and convert it to UTC */
        now_raw = time(NULL);
        now = gmtime(&now_raw);

        /* Fill in the timestamp as "YYYYMMDD HHMMSS" */
        snprintf(tftf_hdr->build_timestamp,
                 sizeof(tftf_hdr->build_timestamp),
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
int tftf_section_collisions(tftf_header * tftf_hdr,
                            tftf_section_descriptor * section,
                            uint32_t * collisions,
                            uint32_t  max_collisions) {
    int num_collisions = 0;

    if (!tftf_hdr || !collisions) {
        fprintf(stderr, "ERROR(tftf_section_collisions): invalid parameters\n");
    } else {
        uint32_t index;
        tftf_section_descriptor * sweeper;

        /* Skip comparing any section marked as to be ignored */
        if (section->section_load_address != DATA_ADDRESS_TO_BE_IGNORED) {
            for (index = 0, sweeper = tftf_hdr->sections;
                 sweeper->section_type != TFTF_SECTION_END;
                 index++, sweeper++) {
                /**
                 * Skip over ourself and any sections with an address of
                 * DATA_ADDRESS_TO_BE_IGNORED
                 */
                if ((sweeper != section) &&
                    (sweeper->section_load_address !=
                     DATA_ADDRESS_TO_BE_IGNORED)) {
                    if (regions_overlap(section->section_load_address,
                                        section->section_length,
                                        sweeper->section_load_address,
                                        sweeper->section_length)) {
                        collisions[num_collisions++] = index;
                    }
                }
            }
        }
    }

    return num_collisions;
}
