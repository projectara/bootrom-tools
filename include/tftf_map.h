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

#ifndef _COMMON_TFTF_MAP_H
#define _COMMON_TFTF_MAP_H


/**
 * @brief Write the TFTF field offsets to an open map file.
 *
 * Append the map for this TFTF to the map file.
 *
 * @param tftf_hdr The TFTF blob to write
 * @param prefix Optional prefix for each map entry
 * @param offset The starting offset of the TFTF (zero for a standalone
 *        tftf map; non-zero for a TFTF in an FFFF).
 * @param map_file The open file object for the map file.
 *
 * @returns Returns nothing.
 */
void write_tftf_map(const tftf_header * tftf_hdr,
                    const char * prefix,
                    uint32_t offset,
                    FILE * map_file);


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
                         const char * output_filename);

#endif /* !_COMMON_TFTF_MAP_H */

