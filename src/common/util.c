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
 * @brief: This file contains the code for general utility functions
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "util.h"


/**
 * @brief Determine if a number is a power of 2
 *
 * @param x The number to check
 *
 * @returns True if x is 2**n, false otherwise
 */
bool is_power_of_2(uint32_t x) {
    return ((x != 0) && !(x & (x - 1)));
}


/**
 * @brief Determine if a number is block-aligned
 *
 * @param location The address to check
 * @param block_size The block size (assumed to be (2**n).
 *
 * @returns True if location is block-aligned, false otherwise
 */
bool block_aligned(uint32_t location, uint32_t block_size) {
    return ((location & (block_size - 1)) == 0);
}


/**
 * @brief Round an address up to the next block boundary, if needed.
 *
 * @param location The address to align
 * @param block_size The block size (assumed to be (2**n).
 *
 * @returns True if location is block-aligned, false otherwise
 */
bool next_boundary(uint32_t location, uint32_t block_size) {
    return (location + (block_size - 1)) & ~(block_size - 1);
}


/**
 * @brief Determine if a region is filled with a constant byte
 *
 * Sort of a "memset" in reverse (is_memset? memcmpf?)
 *
 * @param buf The address to check
 * @param length The length of the region (in bytes) to check
 * @param fill_byte The constant value with which the region should be filled
 *
 * @returns True if locaton is block-aligned, false otherwise
 */
bool is_constant_fill(const uint8_t * buf, const uint32_t length,
                      const uint8_t fill_byte) {
    const uint8_t * limit = &buf[length];
    while (buf < limit) {
        if (*buf++ != fill_byte) {
            return false;
        }
    }

    return true;
}


/**
 * @brief Determine if a string ends with another string
 *
 * @param str The string to test
 * @param suffix The string to check for
 *
 * @returns True str ends with suffix, false otherwise
 */
bool endswith(const char * str, const char * suffix) {
    bool found = false;
    size_t len_str;
    size_t len_suffix;

    if (str && suffix) {
        len_str = strlen(str);
        len_suffix = strlen(suffix);
        if (len_str > len_suffix) {
            found = (strcmp(str, suffix) == 0);
        }
    }

    return found;
}


/**
 * @brief Determine if 2 buffers overlap
 *
 * @param x The 1st buffer
 * @param xlen The length of the 1st buffer
 * @param y The 2nd buffer
 * @param ylen The length of the 2nd buffer
 *
 * @returns True if the buffers overlap, false if they don't
 */
bool overlap(const uint8_t * x, const size_t xlen,
             const uint8_t * y, const size_t ylen) {
    const uint8_t * x_end = x + xlen;
    const uint8_t * y_end = y + ylen;
    return ((y < x_end) && (x < y_end));
}


/**
 * @brief Determine if 2 described regions overlap
 *
 * @param x The 1st buffer
 * @param xlen The length of the 1st buffer
 * @param y The 2nd buffer
 * @param ylen The length of the 2nd buffer
 *
 * @returns True if the buffers overlap, false if they don't
 */
bool regions_overlap(const size_t x, const size_t xlen,
                     const size_t y, const size_t ylen) {
    const size_t x_end = x + xlen;
    const size_t y_end = y + ylen;
    return ((y < x_end) && (x < y_end));
}


/**
 * @brief Mostly-safe string copy (bounded)
 *
 * Lightweight safer_strncpy, does not check for unterminated or overlapped
 * strings.
 *
 * @param dest The buffer to copy into
 * @param destsz The size of the destination buffer
 * @param src The string to copy
 * @param count The maximum number of bytes to copy
 *
 * @returns True if the string was fully copied, false otherwise
 */
bool safer_strncpy(char * dest, size_t destsz, const char * src, size_t count) {
    bool success = true;

    if ((dest) && (destsz > 0) && (src) && (count > 0)) {
        if (count >= destsz) {
            size_t src_length = strlen(src);

            /**
             * In case count is greater than the actual src length, check
             * so see if the actual length of the src string will fit in
             * the buffer. If so, We're OK with the strncpy because it will
             */
            if (src_length < destsz) {
                /* src will fit */
                count = src_length;
            } else {
                /* trim the count to fit, leaving room for the terminator */
                count = destsz - 1;
                success = false;
            }
        }
        strncpy(dest, src, count);
        dest[count] = '\0';
    } else {
        success = false;
    }

    return success;
}


/**
 * @brief Mostly-safe string copy
 *
 * Lightweight safer_strcpy, does not check for unterminated or overlapped
 * strings.
 *
 * @param dest The buffer to copy into
 * @param destsz The size of the destination buffer
 * @param src The string to copy
 *
 * @returns True if the string was fully copied, false otherwise
 */
bool safer_strcpy(char * dest, size_t destsz, const char * src) {
    return (src)? safer_strncpy(dest, destsz, src, strlen(src)) : false;
}


/**
 * @brief Mostly-safe string catenate (bounded)
 *
 * Lightweight safer_strncat, does not check for unterminated or overlapped
 * strings.
 *
 * @param dest The buffer to append to
 * @param destsz The size of the destination buffer
 * @param src The string to catenate
 * @param count The maximum number of bytes to copy
 *
 * @returns True if the string was fully copied, false otherwise
 */
bool safer_strncat(char * dest, size_t destsz, const char * src, size_t count) {
    bool success = true;

    if (dest && (destsz > 0) && src && (count > 0)) {
        size_t usedlen = strlen(dest);
        size_t availsz = destsz - usedlen;

        if (count >= availsz) {
            size_t src_length = strlen(src);

            /**
             * In case count is greater than the actual src length, check
             * so see if the actual length of the src string will fit in
             * the buffer. If so, We're OK with the strncpy because it will
             */
            if (src_length < availsz) {
                /* src will fit */
                count = src_length;
            } else {
                /* trim the count to fit, leaving room for the terminator */
                count = availsz - 1;
                success = false;
            }
        }
        strncpy(&dest[usedlen], src, count);
        dest[count] = '\0';
    } else {
        success = false;
    }

    return success;
}


/**
 * @brief Mostly-safe string catnenation
 *
 * Lightweight strcat_s, does not check for unterminated or overlapped
 * strings.
 *
 * @param dest The buffer to append to
 * @param destsz The size of the destination buffer
 * @param src The string to catenate
 *
 * @returns True if the string was fully copied, false otherwise
 */
bool strcat_s(char * dest, size_t destsz, const char * src) {
    return (src != NULL)? safer_strncat(dest, destsz, src, strlen(src)) :
                          false;
}


/**
 * @brief Change the extension on a pathname
 *
 * @param pathbuf The buffer in which to assemble the path
 * @param pathbuf_length The length of pathbuf, in bytes
 * @param filename The starting filename
 * @param extension The desired extension (w/o ".")
 *
 * @returns Returns true on success, false on error or if the resulting
 * string is truncated.
 */
bool change_extension(char * pathbuf, size_t pathbuf_length,
                      const char * filename, const char * extension) {
    size_t len_root;
    size_t len_ext;
    size_t len_final;
    char * separator;

    if (!pathbuf || (pathbuf_length == 0) || !filename || !extension) {
        errno = EINVAL;
        return false;
    }

    len_root = strlen(filename);
    len_ext = strlen(extension);
    separator = strrchr(filename, '.');
    if (!separator) {
        /* file has no extension */
        len_final = len_root + 1 + len_ext;
        if (len_final >= pathbuf_length) {
            return false;
        }
        snprintf(pathbuf, pathbuf_length, "%s.%s", filename, extension);
    } else {
        /* file has an extension */
        len_root = separator - filename;
        len_final = len_root + 1 + len_ext;
        if (len_final >= pathbuf_length) {
            return false;
        }
        strncpy(pathbuf, filename, len_root + 1); /* (copy the "." too) */
        strcat(pathbuf, extension);
    }
    return true;
}


/**
 * @brief Convert a buffer into its hex representation
 *
 * @param buf The buffer to display
 * @param buflen The length of the buffer
 * @param hexbuf The text buffer in which to put the hex
 * @param hexbuflen The length of the text buffer
 *
 * @returns Returns the formatted buffer.
 */
char * hexlify(uint8_t * buf, size_t buflen, char * hexbuf, size_t hexbuflen) {
    /* Ensure the text buffer is terminated */
    if (hexbuf) {
        hexbuf[0] = '\n';
    }

    if ((buf) && (buflen > 0) && (hexbuf) && (hexbuflen > 0)) {
        size_t src;
        size_t dst;
        size_t dst_remaining = hexbuflen - 3;
        int len;

        for (src = 0, dst = 0; (src < buflen) && (dst < hexbuflen); src++) {
            len = snprintf(&hexbuf[dst], dst_remaining, "%02x", buf[src]);
            dst += len;
            dst_remaining -= len;
        }
    }
    return hexbuf;
}


/**
 * @brief Display a binary blob in 32-byte lines.
 *
 * @param blob The buffer to display
 * @param length The length of the buffer
 * @param show_all If true, then the entire blob is displayed. Otherwise,
 *        up to 3 lines are displayed, and if the blob is more than 96
 *        bytes long, only the first and last 32 bytes are displayed, with
 *        a ":" between them.
 * @param indent An optional prefix string
 *
 * @returns Nothing
 */
void display_binary_data(uint8_t * blob, size_t length, bool show_all, char * indent) {
    if ((blob) && (length > 0)) {
        const size_t max_on_line = 32;
        size_t start;
        char linebuf[256];


        /* Make missing indents print nicely */
        if (!indent) {
            indent = "";
        }

        /* Print the data blob*/
        if (show_all || (length <= (3 * max_on_line))) {
            /* Nominally a small blob */
            for (start = 0; start < length; start += max_on_line) {
                size_t num_bytes = min(length, max_on_line);
                printf("%s%s", indent, hexlify(&blob[start], num_bytes,
                                               linebuf, sizeof(linebuf)));
            }
        } else {
            /**
             * Blob too long, so print just the first and last lines,
             * separated by a ":".
             */
            printf("%s%s\n", indent, hexlify(blob, max_on_line,
                                             linebuf, sizeof(linebuf)));
            printf("%s  :\n", indent);
            start = length - max_on_line;
            printf("%s%s\n", indent, hexlify(&blob[start], max_on_line,
                                             linebuf, sizeof(linebuf)));
        }
    }
}

