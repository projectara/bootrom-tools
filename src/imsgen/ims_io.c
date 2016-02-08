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
 * @brief: This file contains the I/O routines for IMS testing
 *
 */

#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <libgen.h>
#include <openssl/evp.h>
#include <sqlite3.h>
#include "util.h"
#include "mcl_arch.h"
#include "mcl_oct.h"
#include "mcl_ecdh.h"
#include "mcl_rand.h"
#include "mcl_rsa.h"
#include "crypto.h"
#include "db.h"
#include "ims_common.h"
#include "ims_io.h"

/* Uncomment the following define to enable IMS diagnostic messages */
/*#define IMS_DEBUGMSG*/


/**
 * @brief Determine how many IMS values are in an IMS file
 *
 * @param ims_filename The name of the IMS input file
 *
 * @returns If successful, the number (0-n) of IMS values in the file.
 *          Otherwise, a negative number.
 */
int num_ims_in_file(const char * ims_filename) {
    struct stat ims_stat = {0};
    int num_ims = -1;

    if (stat(ims_filename, &ims_stat) != 0) {
        fprintf(stderr, "ERROR: Can't find IMS file '%s'\n", ims_filename);
        num_ims = -1;
    } else {
        num_ims = ims_stat.st_size / (IMS_LINE_SIZE);
    }

    return num_ims;
}


/**
 * @brief Get an IMS value from a string or the Nth entry in an IMS file
  *
 * Note that if ims_binascii is non-null, ims_filename and ims_index are
 * ignored. If ims_value is null, we use ims_filename and ims_index.
 *
 * @param ims_value (optional) The binascii string representation of the IMS
 * @param ims_filename (optional) The name of the IMS input file
 * @param ims_index (optional) Which IMS value to use from ims_filename
 * @param ims Where to store the IMS
 *
 * @returns 0 if successful, errno or -1 otherwise.
 */
int get_ims(const char * ims_binascii,
            const char * ims_filename,
            const int    ims_index,
            uint8_t      ims[IMS_SIZE]) {
    int status = 0;

    if (ims_binascii) {
        /* Parse the IMS from the string provided */
        if (strlen(ims_binascii) != IMS_BINASCII_SIZE) {
            fprintf(stderr, "ERROR: Invalid IMS value (length)\n");
            status = EINVAL;
        } else {
            status = ims_parse(ims_binascii, ims);
        }
    } else {
        /* Extract the IMS from the Nth entry in the IMS file */
        int  num_available_ims = num_ims_in_file(ims_filename);

        if (num_available_ims <= 0) {
            fprintf (stderr, "ERROR: No IMS in IMS file\n");
        } else {
            if ((ims_index < 0) ||(ims_index > num_available_ims)) {
                fprintf(stderr,
                        "ERROR: ims-index must be in the range 0..%d\n",
                        num_available_ims - 1);
                status = EINVAL;
            } else {
                int ims_fd;

                off_t offset = ims_index * IMS_LINE_SIZE;
                ims_fd = open(ims_filename, O_RDONLY);
                if (ims_fd  == -1) {
                    fprintf(stderr, "ERROR: Can't open IMS file '%s'\n", ims_filename);
                    status = errno;
                } else {
                    status = ims_read(ims_fd, offset, ims);
                    close(ims_fd);
                }
            }
        }
    }

    return status;
}


/**
 * @brief Write a blob to a named file
 *
 * @param fname The pathanme of the file to create
 * @param buf Points to the buffer to write.
 * @param len The length in bytes of the buffer.
 *
 * @returns 0 if successful, errno or -1 otherwise.
 */
int write_blob(const char * fname, const uint8_t * buf, const size_t len) {
    int status = 0;
    int fd;

    fd = creat(fname, 0666);
    if (fd == -1) {
        fprintf(stderr, "ERROR: can't create %s (err %d)\n", fname, errno);
        status = errno;
    } else {
        if (write(fd, buf, len) != len) {
            fprintf(stderr, "ERROR: Problems writing %s (err %d)\n",
                    fname, errno);
            status = errno;
        }
        close(fd);
    }

    return status;
}


/**
 * @brief Write an array of octets to a named file
 *
 * @param fname The pathanme of the file to write
 * @param octet Points to an array of octets to write.
 * @param num_octets The number of octets in the octets array.
 *
 * @returns 0 if successful, errno or -1 otherwise.
 */
int write_octets(const char * fname, mcl_octet * octet, int num_octets) {
    int status = 0;
    int i;
    int fd;

    fd = creat(fname, 0666);
    if (fd == -1) {
        fprintf(stderr, "ERROR: can't create %s (err %d)\n", fname, errno);
        status = errno;
    } else {
        for (i = 0; i < num_octets; i++, octet++) {
            if (write(fd, octet->val, octet->len) != octet->len) {
                fprintf(stderr, "ERROR: Problems writing %s (err %d)\n",
                        fname, errno);
                status = errno;
            }
        }
        close(fd);
    }

    return status;
}


/**
 * @brief Read an array of octets from a named file
 *
 * NOTE: The file contents are divided evenly between the N octets.
 *
 * @param fname The pathanme of the file to read
 * @param octet Points to an array of octets to read.
 * @param num_octets The number of octets in the octets array.
 *
 * @returns 0 if successful, errno or -1 otherwise.
 */
int read_octets(const char * fname, mcl_octet * octet, int num_octets) {
    int status = 0;
    int fd;
    int i;
    size_t bytes_to_read = num_octets;
    ssize_t bytes_read;
    struct stat file_info = {0};

    if (num_octets < 1) {
        return EINVAL;
    }

    if (stat(fname, &file_info) != 0) {
        fprintf(stderr, "ERROR: Can't find '%s'\n", fname);
        status = errno;
    } else {
        bytes_to_read = file_info.st_size / num_octets;

        fd = open(fname, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "ERROR: can't open %s (err %d)\n", fname, errno);
            status = errno;
        } else {
            for (i = 0; i < num_octets; i++, octet++) {
                bytes_read = read(fd, octet->val, bytes_to_read);
                octet->len = bytes_read;
                if (bytes_read != bytes_to_read) {
                    fprintf(stderr, "ERROR: Problems reading %s, got %d/%d (err %d)\n",
                            fname, (int)bytes_read, (int)bytes_to_read, errno);
                    status = errno;
                }
            }
            close(fd);
        }
    }

    return status;
}


/**
 * @brief Read a file into an octet
 *
 * Allocates a buffer for an octet and reads a file into it.
 *
 * @param ims_filename The name of the IMS input file
 * @param octet The octet to load
 *
 * @returns If successful, the number (0-n) of IMS values in the file.
 *          Otherwise, a negative number.
 */
int read_file_into_octet(const char * filename, mcl_octet *octet) {
    int         status = 0;
    int         fd;
    struct stat file_info = {0};
    ssize_t     bytes_read;

    /* Sanity check: params valid and octet doesn't have a buffer */
    if (!filename || !octet || octet->val) {
        return EINVAL;
    }

    if (stat(filename, &file_info) != 0) {
        fprintf(stderr, "ERROR: Can't find '%s'\n", filename);
        status = errno;
    } else {
        fd = open(filename, O_RDONLY);
        if (fd  == -1) {
            fprintf(stderr, "ERROR: Can't open '%s'\n", filename);
            status = errno;
        } else {
            octet->val = malloc(file_info.st_size);
            if (!octet->val) {
                fprintf(stderr, "ERROR: Can't allocate a buffer for '%s'\n",
                        filename);
                status = ENOMEM;
            } else {
                octet->max = file_info.st_size;
                bytes_read = read(fd, octet->val, file_info.st_size);
                if (bytes_read != file_info.st_size) {
                    fprintf(stderr, "ERROR: Can't read '%s'\n", filename);
                    status = EIO;
                }
                octet->len = bytes_read;
            }
            close (fd);
        }
    }

    return status;
}
