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
#include "ffff_out.h"


/**
 * @brief Write the FFFF blob to the output file.
 *
 * Create a FFFF file from the FFFF blob.
 *
 * @param ffff_romimage The FFFF romimage to write
 * @param output_filename Pathname to the FFFF output file.
 *
 * @returns Returns true on success, false on failure.
 */
bool write_ffff_file(const struct ffff * ffff_romimage,
                     const char * output_filename) {
    bool success = false;
    int ffff_fd = -1;
    const ffff_element_descriptor * element;
    size_t bytes_written;

    if ((ffff_romimage == NULL) || (output_filename == NULL)) {
        fprintf(stderr, "ERROR (write_ffff_file): Invalid args\n");
        errno = EINVAL;
    } else {
        /* Write the blob to the file */
        ffff_fd = open(output_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (ffff_fd < 0) {
            fprintf(stderr, "Error: unable to create '%s' (err %d)\n",
                    output_filename,
                    ffff_fd);
        } else {
            bytes_written =
                write(ffff_fd, ffff_romimage->blob, ffff_romimage->blob_length);
            if (bytes_written != ffff_romimage->blob_length) {
                fprintf(stderr, "Error: Only wrote %u/%u bytes to '%s' (err %d)\n",
                        (uint32_t)bytes_written,
                        (uint32_t)ffff_romimage->blob_length,
                        output_filename,
                        errno);
            } else {
                fprintf(stderr, "Wrote %u bytes to '%s'\n",
                        (uint32_t)ffff_romimage->blob_length, output_filename);
                success = true;
            }

            /* Done with the FFFF file. */
            close(ffff_fd);
        }
    }

    return success;
}

