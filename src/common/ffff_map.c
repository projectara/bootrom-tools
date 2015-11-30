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
 * @brief: This file contains the includes for writing a FFFF file.
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "util.h"
#include "ffff.h"
#include "ffff_common.h"
#include "ffff_map.h"
#include "ffff_print.h"
#include "tftf.h"


/**
 * @brief Write the FFFF element table field offsets to the map file.
 *
 * Append the map for this FFFF to the map file.
 *
 * @param ffff_hdr The FFFF blob to write
 * @param prefix Optional prefix for each map entry
 * @param offset The starting offset of the FFFF (zero for a standalone
 *        ffff map; non-zero for a FFFF in an FFFF).
 * @param map_file The open file object for the map file.
 *
 * @returns Returns nothing
 */
static void write_ffff_element_table_map(const ffff_header * ffff_hdr,
                                         const char * prefix,
                                         size_t offset,
                                         FILE * map_file) {
    int index;
    const ffff_element_descriptor * element = ffff_hdr->elements;

    offset += offsetof(ffff_header, elements);
    fprintf(map_file,"%selement_table  %08x\n", prefix, (uint32_t)offset);

    /* Print out the occupied entries in the element table */
    for (index = 0;
         index < ffff_max_elements;
         index++, element++, offset += sizeof(ffff_element_descriptor)) {
        fprintf(map_file, "%selement[%d].type  %08x\n",
                prefix, index, (uint32_t)(offset));
        fprintf(map_file, "%selement[%d].class  %08x\n",
                prefix, index, (uint32_t)(offset + 1));
        fprintf(map_file, "%selement[%d].id  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(ffff_element_descriptor,
                                             element_id)));
        fprintf(map_file, "%selement[%d].length  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(ffff_element_descriptor,
                                             element_length)));
        fprintf(map_file, "%selement[%d].location  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(ffff_element_descriptor,
                                             element_location)));
        fprintf(map_file, "%selement[%d].generation  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(ffff_element_descriptor,
                                             element_generation)));
    }
}


/**
 * @brief Write the FFFF element table field offsets to the map file.
 *
 * Append the map for this FFFF to the map file.
 *
 * @param ffff_hdr The FFFF blob to write
 * @param prefix Optional prefix for each map entry
 * @param offset The starting offset of the FFFF (zero for a standalone
 *        ffff map; non-zero for a FFFF in an FFFF).
 * @param map_file The open file object for the map file.
 *
 * @returns Returns nothing
 */
void write_ffff_element_table_payload_map(const ffff_header * ffff_hdr,
                                          const char * prefix,
                                          const uint8_t * blob,
                                          FILE * map_file) {
    const ffff_element_descriptor * element = ffff_hdr->elements;
    const tftf_header * tftf_hdr;
    uint32_t index;
    char tftf_prefix[256];

    for (index = 0; index < ffff_max_elements; index++, element++) {
        tftf_hdr = (const tftf_header *)&blob[element->element_location];
        sprintf(tftf_prefix, "%s.element[%d].%s",
                prefix, index, ffff_element_type_name(element->element_type));

        if ((element->element_location != 0) &&
            (sniff_tftf_header(tftf_hdr))) {
            /* Map out the TFTF backing that element */
            write_tftf_map(&blob[element->element_location],
                           tftf_prefix, element->element_location, map_file);
        } else {
            /* Unrecognized type, just print where it is */
            fprintf(map_file, "%s %08x\n",
                    tftf_prefix, element->element_location);
        }
        if (element->element_type == FFFF_ELEMENT_END) {
            break;
        }
    }
}

/**
 * @brief Write the FFFF header field offsets to the map file.
 *
 * Append the map for this FFFF to the map file.
 *
 * @param ffff_hdr The FFFF blob to write
 * @param prefix Optional prefix for each map entry
 * @param offset The starting offset of the FFFF (zero for a standalone
 *        ffff map; non-zero for a FFFF in an FFFF).
 * @param map_file The open file object for the map file.
 *
 * @returns Returns nothing
 */
void write_ffff_header_map(const ffff_header * ffff_hdr,
                           const char * prefix,
                           uint32_t offset,
                           FILE * map_file) {
    int index;
    char prefix_buf[256] = {0};

    prefix_buf[0] = '\0';
    if (prefix) {

        /**
         * If we have a prefix, then issue a marker (without any added "."
         * separator) for the start of the ffff.
         */
        fprintf(map_file, "%s  %08x\n", prefix, offset);

        /* Ensure the prefix we use for the FFFF fields has a separator */
        if (!endswith(prefix, ".")) {
            snprintf(prefix_buf, sizeof(prefix_buf), "%s.", prefix);
            prefix = prefix_buf;
        }
    }


    /* Print out the fixed part of the header */
    fprintf(map_file, "%ssentinel  %08x\n",
            prefix,
            (uint32_t)(offset + offsetof(ffff_header, sentinel_value)));
    fprintf(map_file, "%stime_stamp  %08x\n",
            prefix,
            (uint32_t)(offset + offsetof(ffff_header, build_timestamp)));
    fprintf(map_file, "%simage_name  %08x\n",
            prefix,
            (uint32_t)(offset + offsetof(ffff_header, flash_image_name)));
    fprintf(map_file, "%sflash_capacity  %08x\n",
            prefix, (uint32_t)(offset + offsetof(ffff_header, flash_capacity)));
    fprintf(map_file, "%serase_block_size  %08x\n",
            prefix, (uint32_t)(offset + offsetof(ffff_header, erase_block_size)));
    fprintf(map_file, "%sheader_size  %08x\n",
            prefix, (uint32_t)(offset + offsetof(ffff_header, header_size)));
    fprintf(map_file, "%simage_length  %08x\n",
           prefix, (uint32_t)(offset + offsetof(ffff_header, flash_image_length)));
    fprintf(map_file, "%sgeneration  %08x\n",
            prefix, (uint32_t)(offset + offsetof(ffff_header, header_generation)));
    for (index = 0; index < FFFF_RESERVED; index++) {
        fprintf(map_file, "%sreserved[%d]  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(ffff_header, reserved[index])));
    }

    /* Print out the variable part of the header */
    write_ffff_element_table_map(ffff_hdr, prefix, offset, map_file);

    /* Print out the tail sentinel */
    fprintf(map_file, "%stail_sentinel  %08x\n",
            prefix,
            (uint32_t)(offset + (ffff_hdr->header_size -
                                 sizeof(ffff_hdr->trailing_sentinel_value))));

}


/**
 * @brief Create a map file and write the FFFF field offsets to it
 *
 * Create a FFFF map file from the FFFF romimage.
 *
 * @param romimage The FFFF to write
 * @param output_filename Pathname to the FFFF output file.
 *
 * @returns Returns true on success, false on failure.
 */
bool write_ffff_map_file(const struct ffff * romimage,
                         const char * output_filename) {
    bool success = false;
    char map_filename[MAXPATH];
    FILE * map_file = NULL;
    ffff_header * ffff_hdr = NULL;
    ffff_element_descriptor * element;
    uint32_t index;
    uint32_t offset;
    char tftf_prefix[256];


    if ((romimage == NULL) || (output_filename == NULL)) {
        fprintf(stderr, "ERROR (write_ffff_file): Invalid args\n");
        errno = EINVAL;
    } else {
        /* Convert the filename.xxx to filename.map if needed */
        success =  change_extension(map_filename, sizeof(map_filename),
                                    output_filename, ".map");
        if (!success) {
            fprintf(stderr, "ERROR: map file path too long\n");
        } else {
            map_file = fopen(map_filename, "w");
            if (map_file == NULL) {
                fprintf(stderr, "ERROR: Can't create map file '%s' (err %d)\n",
                        map_filename, errno);
            } else {
                offset = (uint32_t)((uint8_t*)(romimage->ffff_hdrs[0]) - romimage->blob);
                write_ffff_header_map(romimage->ffff_hdrs[0], "ffff[0]",
                                     offset, map_file);
                /**
                 * If it is a corrupted FFFF or one which was caught in
                 * mid-update, there may only be 1 valid FFFF header.
                 */
                if (romimage->ffff_hdrs[1]) {
                    /* Valid 2nd header */
                    offset = (uint32_t)((uint8_t*)(romimage->ffff_hdrs[1]) - romimage->blob);
                    write_ffff_header_map(romimage->ffff_hdrs[1], "ffff[1]",
                                         offset, map_file);

                    /**
                     * If both element tables match, we can get by with
                     * printing it only once
                     */
                    if (ffff_element_tables_match(romimage->ffff_hdrs[0],
                                                  romimage->ffff_hdrs[1])) {
                        /* Print the 2 tables unified as 2 1 */
                        write_ffff_element_table_payload_map(romimage->ffff_hdrs[0],
                                                             "ffff",
                                                             romimage->blob,
                                                             map_file);
                    } else {
                        /* Print each element table separately */
                        write_ffff_element_table_payload_map(romimage->ffff_hdrs[0],
                                                             "ffff[0]",
                                                             romimage->blob,
                                                             map_file);
                        write_ffff_element_table_payload_map(romimage->ffff_hdrs[1],
                                                             "ffff[1]",
                                                             romimage->blob,
                                                             map_file);
                    }
                } else {
                    /* Only 1 valid header to display */
                    write_ffff_element_table_payload_map(romimage->ffff_hdrs[0],
                                                         "ffff[0]",
                                                         romimage->blob,
                                                         map_file);
                }

                /* Done with the FFFF file. */
                fclose(map_file);
            }
        }
    }

    return success;
}

