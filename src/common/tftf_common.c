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
#include "tftf_print.h"

/**
 * This contains the maximum number of sections in the header.
 * NOTE: The initial value is a valid number for the minimum TFTF header
 * size, but it is also a placeholder - main() in any app that uses
 * tftf_max_sections is responsible for recalculating it as soon as it has
 * determined the header size (either from a parameter or parsing a TFTF
 * file/blob).
 */
uint32_t tftf_max_sections =
        CALC_MAX_TFTF_SECTIONS(TFTF_HEADER_SIZE_DEFAULT);


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
 * @brief Append to a TFTF blob
 *
 * Expand a TFTF blob, returning a new, larger blob. (The old blob is freed.)
 *
 * @param tftf_hdr Pointer to the TFTF header to examine
 * @param data (optional)Pointer to the data to append (will zero the extended
 *        region if not supplied)
 * @param extra_size The size of the data to append
 *
 * @returns If successful, returns a pointer to a new blob with
 *          the old blob copied into it (the old blob is freed).
 *          If not successful, returns NULL.
 */
tftf_header * append_to_tftf_blob(tftf_header * tftf_hdr,
                                  const uint8_t * data,
                                  const size_t extra_size) {
    tftf_header * new_tftf_hdr = NULL;

    if (tftf_hdr) {
        size_t length = tftf_hdr->header_size + tftf_payload_size(tftf_hdr);
        new_tftf_hdr = malloc (length + extra_size);
        if (!new_tftf_hdr) {
            fprintf(stderr, "ERROR: Can't allocate an extended TFTF\n");
        } else {
            /* Copy the old TFTF blob into the new one */
            memcpy(new_tftf_hdr, tftf_hdr, length);

            if (data) {
                /* Copy the extended bytes */
                memcpy(((uint8_t*)new_tftf_hdr) + length, data, extra_size);
            } else {
                /* Zero out the extended bytes */
                memset(((uint8_t*)new_tftf_hdr) + length, 0, extra_size);
            }

            /* Dispose of the old blob */
            free_tftf_header(tftf_hdr);
        }
    }

    return new_tftf_hdr;
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
int tftf_section_collisions(const tftf_header * tftf_hdr,
                            const tftf_section_descriptor * section,
                            uint32_t * collisions,
                            const uint32_t  max_collisions) {
    int num_collisions = 0;

    if (!tftf_hdr || !collisions) {
        fprintf(stderr, "ERROR(tftf_section_collisions): invalid parameters\n");
    } else {
        uint32_t index;
        const tftf_section_descriptor * sweeper;

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


/**
 * @brief Determine the signable region of a TFTF
 *
 * @param tftf_hdr Pointer to the TFTF header to examine
 * @param hdr_pstart Pointer to a ptr variable that will be set to the start
 *                   of the signable region in the TFTF header.
 * @param hdr_length Pointer to a variable which will be set to the length
 *                   of the signable region in the TFTF header.
 * @param scn_pstart Pointer to a ptr variable that will be set to the start
 *                   of the signable region in the TFTF sections.
 * @param scn_length Pointer to a variable which will be set to the length
 *                   ofthe signable region in the TFTF sections.
 *
 * @returns True on success, false on failure
 */
bool tftf_get_signable_region(tftf_header * tftf_hdr,
                              uint8_t ** hdr_pstart, size_t * hdr_length,
                              uint8_t ** scn_pstart, size_t * scn_length) {
    tftf_section_descriptor * section;
    if (!tftf_hdr || !hdr_pstart || !hdr_length ||
        !scn_pstart || !scn_length) {
        return false;
    }

    /* Find the signable region of the header & section payload */
    *hdr_pstart = (uint8_t*)tftf_hdr;
    *hdr_length = 0;
    *scn_pstart = SECTION_PAYLOAD_START(tftf_hdr);
    *scn_length = 0;
    for (section = tftf_hdr->sections;
         section->section_type < TFTF_SECTION_SIGNATURE;
         section++) {
        *hdr_length = (uint8_t*)section + sizeof(*section) - (uint8_t *)tftf_hdr;
        *scn_length += section->section_length;
    }

    return true;
}


/**
 * @brief Append a section to a TFTF
 *
 * If successful, the old TFTF blob is freed and a new one created.
 *
 * @param ptftf_hdr Pointer to the pointer to the existing TFTF blob.
 *        If we can add the new section to the TFTF, then we allocate
 *        a new (bigger) blob, copy the old blob to it and append the
 *        data.
 * @param type The section Type
 * @param class The section Class
 * @param id The section ID
 * @param load_address The section load address
 * @param data A pointer to the section payload blob
 * @param length The size of the payload blob pointed to by "data"
 *
 * @returns True on success, false on failure
 */
bool tftf_add_section(tftf_header ** ptftf_hdr, uint32_t type, uint32_t class,
                      uint32_t id, uint32_t load_address, uint8_t *data,
                      size_t length) {
    tftf_header * tftf_hdr = NULL;
    int i;
    bool restricted = false;
    bool success = true;

    /* Sanity check */
    if (!ptftf_hdr || !*ptftf_hdr) {
        fprintf (stderr, "ERROR (tftf_add_section): invalid parameters\n");
        return false;
    } else {
        tftf_hdr = *ptftf_hdr;
    }

    /**
     *  Find the end-of-table entry and check for any section types
     *  that would restrict which types of sections we can append.
     */
    for (i = 0;
         ((i < tftf_max_sections) &&
          (tftf_hdr->sections[i].section_type != TFTF_SECTION_END));
         i++) {
        if (tftf_hdr->sections[i].section_type >= TFTF_SECTION_SIGNATURE) {
            restricted = true;
        }
    }
    /* Check to see if we can add the section */
    if (i >= (tftf_max_sections - 2)) {
        fprintf(stderr, "ERROR: TFTF section table is full\n");
        success = false;
    } else if (restricted && (type < TFTF_SECTION_SIGNATURE)) {
        fprintf(stderr,
                "ERROR: You can't add a %s after a signature or certificate\n",
                tftf_section_type_name(type));
        success = false;
    }

    if (success) {
         /* Allocate a larger blob for the expanded TFTF */
        tftf_hdr = append_to_tftf_blob(tftf_hdr, data, length);
        if (tftf_hdr) {
            /* Make the caller point to the new blob */
            *ptftf_hdr = tftf_hdr;

            /**
             *  Copy the end-of-table marker to the next slot and overwrite the
             *  old end-of-tble marker with the new section info.
             */
            tftf_hdr->sections[i+1] = tftf_hdr->sections[i];
            tftf_hdr->sections[i].section_type = type;
            tftf_hdr->sections[i].section_class = class;
            tftf_hdr->sections[i].section_id = id;
            tftf_hdr->sections[i].section_load_address = load_address;
            tftf_hdr->sections[i].section_length = (uint32_t)length;
            tftf_hdr->sections[i].section_expanded_length = (uint32_t)length;
        }
    }

    return success;
}
