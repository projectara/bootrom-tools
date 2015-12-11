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
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "util.h"
#include "tftf.h"
#include "tftf_common.h"
#include "tftf_out.h"


/**
 * @brief Write the TFTF blob to the output file.
 *
 * Create a TFTF file from the TFTF blob.
 *
 * @param tftf_hdr The TFTF blob to write
 * @param output_filename Pathname to the TFTF output file.
 *
 * @returns Returns true on success, false on failure.
 */
bool write_tftf_file(const tftf_header * tftf_hdr,
                     const char * output_filename) {
    bool success = true;
    int tftf_fd = -1;
    const tftf_section_descriptor * section;
    size_t bytes_written;
    size_t length;

    if ((tftf_hdr == NULL) || (output_filename == NULL)) {
        fprintf(stderr, "ERROR (write_tftf_file): Invalid args\n");
        errno = EINVAL;
    } else {
        char *path;
        char *scratchpath;

        scratchpath = strdup(output_filename);
        if (!scratchpath) {
            errno = ENOMEM;
            success = false;
        } else {
            path = dirname(scratchpath);
            if (strlen(path) > 1) {
                success = (mkdir_recursive(path) == 0);
            }
            free(scratchpath);
        }
    }

    if (success) {
        /* Determine the length of the blob */
        length = tftf_hdr->header_size + tftf_payload_size(tftf_hdr);
        /*****/printf("tftf length = %x + %x = %x\n", tftf_hdr->header_size, tftf_payload_size(tftf_hdr), length);

        /* Write the blob to the file */
        tftf_fd = open(output_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (tftf_fd < 0) {
            fprintf(stderr, "Error: unable to create '%s' (err %d)\n",
                    output_filename,
                    tftf_fd);
            success = false;
        } else {
            bytes_written = write(tftf_fd, tftf_hdr, length);
            if (bytes_written != length) {
                fprintf(stderr, "Error: unable to write all of '%s' (err %d)\n",
                        output_filename,
                        tftf_fd);
                success = false;
            } else {
                fprintf(stderr, "Wrote %u bytes to '%s'\n",
                        (uint32_t)length, output_filename);
                success = true;
            }

            /* Done with the TFTF file. */
            close(tftf_fd);
        }
    }


    return success;
}

