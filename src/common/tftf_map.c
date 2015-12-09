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
 * @brief: This file contains the includes for writing a TFTF file.
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
#include "tftf.h"
#include "tftf_common.h"
#include "tftf_map.h"


/**
 * @brief Write the TFTF section table field offsets to the map file.
 *
 * Append the map for this TFTF to the map file.
 *
 * @param tftf_hdr The TFTF blob to write
 * @param prefix Optional prefix for each map entry
 * @param offset The starting offset of the TFTF (zero for a standalone
 *        tftf map; non-zero for a TFTF in an FFFF).
 * @param map_file The open file object for the map file.
 *
 * @returns Returns nothing
 */
void write_tftf_section_table_map(const tftf_header * tftf_hdr,
                                  const char * prefix,
                                  size_t offset,
                                  FILE * map_file) {
    int index;
    const tftf_section_descriptor * section = tftf_hdr->sections;

    offset += offsetof(tftf_header, sections);

    /* Print out the occupied entries in the section table */
    for (index = 0;
         index < tftf_max_sections;
         index++, section++, offset += sizeof(*section)) {
        fprintf(map_file, "%ssection[%d].type  %08x\n",
                prefix, index, (uint32_t)(offset));
        fprintf(map_file, "%ssection[%d].class  %08x\n",
                prefix, index, (uint32_t)(offset + 1));
        fprintf(map_file, "%ssection[%d].id  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(tftf_section_descriptor,
                                             section_id)));
        fprintf(map_file, "%ssection[%d].section_length  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(tftf_section_descriptor,
                                             section_length)));
        fprintf(map_file, "%ssection[%d].load_address  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(tftf_section_descriptor,
                                             section_load_address)));
        fprintf(map_file, "%ssection[%d].expanded_length  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(tftf_section_descriptor,
                                             section_expanded_length)));
    }
}


/**
 * @brief Write the TFTF field offsets to the currently open map file.
 *
 * Append the map for this TFTF to the map file.
 *
 * @param tftf_hdr The TFTF blob to write
 * @param prefix Optional prefix for each map entry
 * @param offset The starting offset of the TFTF (zero for a standalone
 *        tftf map; non-zero for a TFTF in an FFFF).
 * @param map_file The open file object for the map file.
 *
 * @returns Returns nothing
 */
void write_tftf_map(const tftf_header * tftf_hdr,
                    const char * prefix,
                    uint32_t offset,
                    FILE * map_file) {
    int index;
    char prefix_buf[256] = {0};

    prefix_buf[0] = '\0';
    if (prefix) {

        /**
         * If we have a prefix, then issue a marker (without any added "."
         * separator) for the start of the tftf.
         */
        fprintf(map_file, "%s  %08x\n", prefix, offset);

        /* Ensure the prefix we use for the TFTF fields has a separator */
        if (!endswith(prefix, ".")) {
            snprintf(prefix_buf, sizeof(prefix_buf), "%s.", prefix);
            prefix = prefix_buf;
        }
    }


    /* Print out the fixed part of the header */
    fprintf(map_file, "%ssentinel  %08x\n",
            prefix,
            (uint32_t)(offset + offsetof(tftf_header, sentinel_value)));
    fprintf(map_file, "%sheader_size  %08x\n",
            prefix, (uint32_t)(offset + offsetof(tftf_header, header_size)));
    fprintf(map_file, "%stimestamp  %08x\n",
            prefix,
            (uint32_t)(offset + offsetof(tftf_header, build_timestamp)));
    fprintf(map_file, "%sfirmware_name  %08x\n",
            prefix,
            (uint32_t)(offset + offsetof(tftf_header, firmware_package_name)));
    fprintf(map_file, "%spackage_type  %08x\n",
           prefix, (uint32_t)(offset + offsetof(tftf_header, package_type)));
    fprintf(map_file, "%sstart_location  %08x\n",
            prefix, (uint32_t)(offset + offsetof(tftf_header, start_location)));
    fprintf(map_file, "%sunipro_mfgr_id  %08x\n",
            prefix, (uint32_t)(offset + offsetof(tftf_header, unipro_mid)));
    fprintf(map_file, "%sunipro_product_id  %08x\n",
            prefix, (uint32_t)(offset + offsetof(tftf_header, unipro_pid)));
    fprintf(map_file, "%sara_vendor_id  %08x\n",
            prefix, (uint32_t)(offset + offsetof(tftf_header, ara_vid)));
    fprintf(map_file, "%sara_product_id  %08x\n",
            prefix, (uint32_t)(offset + offsetof(tftf_header, ara_pid)));
    for (index = 0; index < TFTF_NUM_RESERVED; index++) {
        fprintf(map_file, "%sreserved[%d]  %08x\n",
                prefix, index,
                (uint32_t)(offset + offsetof(tftf_header, reserved[index])));
    }

    /* Print out the variable part of the header */
    write_tftf_section_table_map(tftf_hdr, prefix, offset, map_file);
}


/**
 * @brief Create a map file and write the TFTF field offsets to it
 *
 * Create a TFTF map file from the TFTF blob.
 *
 * @param tftf_hdr The TFTF blob to write
 * @param output_filename Pathname to the TFTF output file.
 *
 * @returns Returns true on success, false on failure.
 */
bool write_tftf_map_file(const tftf_header * tftf_hdr,
                         const char * output_filename) {
    bool success = false;
    char map_filename[MAXPATH];
    FILE * map_file = NULL;

    if ((tftf_hdr == NULL) || (output_filename == NULL)) {
        fprintf(stderr, "ERROR (write_tftf_file): Invalid args\n");
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
                write_tftf_map(tftf_hdr, "tftf[3]", 0, map_file);

                /* Done with the TFTF file. */
                fclose(map_file);
            }
        }
    }

    return success;
}

