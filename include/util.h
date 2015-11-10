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
 * @brief: This file contains the includes for general utility functions
 *
 */

#ifndef _COMMON_UTIL_H
#define _COMMON_UTIL_H

#include <stdint.h>
#include <stdbool.h>


#ifndef MAXPATH
    #define MAXPATH 1024
#endif


/**
 * @brief: returns the minimum of 2 numbers (NOTE: beware of min(x++...)!)
 */
#ifndef min
    #define min(x,y)   (((x) < (y))? (x) : (y))
#endif


/**
 * @brief: returns the offset of a structure element in a structure type
 */
#ifndef offsetof
    #define offsetof(type,member) \
        &((type *)NULL->member));
#endif


/**
 * @brief: returns the number of elements in the array
 */
#define _countof(x)   (sizeof(x)/sizeof((x)[0]))


/**
 * @brief: Set bit N
 */
#ifndef BIT
    #define BIT(n)    (1 << (n))
#endif

/**
 * @brief Determine if a number is a power of 2
 *
 * @param x The number to check
 *
 * @returns True if x is 2**n, false otherwise
 */
bool is_power_of_2(uint32_t x);


/**
 * @brief Determine if a number is block-aligned
 *
 * @param location The address to check
 * @param block_size The block size (2**n)
 *
 * @returns True if locaton is block-aligned, false otherwise
 */
bool block_aligned(uint32_t location, uint32_t block_size);


/**
 * @brief Determine if a region is filled with a constant byte
 *
 * Sort of a "memset" in reverse (is_memset?)
 *
 * @param buf The address to check
 * @param length The length of the region (in bytes) to check
 * @param fill_byte The constant value with which the region should be filled
 *
 * @returns True if locaton is block-aligned, false otherwise
 */
bool is_constant_fill(const uint8_t * buf, const uint32_t length,
                      const uint8_t fill_byte);


/**
 * @brief Determine if a string ends with another string
 *
 * @param str The string to test
 * @param suffix The string to check for
 *
 * @returns True str ends with suffix, false otherwise
 */
bool endswith(const char * str, const char * suffix);


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
             const uint8_t * y, const size_t ylen);


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
                     const size_t y, const size_t ylen);


/**
 * @brief Mostly-safe string copy (bounded)
 *
 * Lightweight strncpy_s, does not check for unterminated or overlapped
 * strings.
 *
 * @param dest The buffer to copy into
 * @param destsz The size of the destination buffer
 * @param src The string to copy
 * @param count The maximum number of bytes to copy
 *
 * @returns True if the string was fully copied, false otherwise
 */
bool strncpy_s(char * dest, size_t destsz, const char * src, size_t count);


/**
 * @brief Mostly-safe string copy
 *
 * Lightweight strcpy_s, does not check for unterminated or overlapped
 * strings.
 *
 * @param dest The buffer to copy into
 * @param destsz The size of the destination buffer
 * @param src The string to copy
 *
 * @returns True if the string was fully copied, false otherwise
 */
bool strcpy_s(char * dest, size_t destsz, const char * src);


/**
 * @brief Mostly-safe string catenate (bounded)
 *
 * Lightweight strncat_s, does not check for unterminated or overlapped
 * strings.
 *
 * @param dest The buffer to append to
 * @param destsz The size of the destination buffer
 * @param src The string to catenate
 * @param count The maximum number of bytes to copy
 *
 * @returns True if the string was fully copied, false otherwise
 */
bool strncat_s(char * dest, size_t destsz, const char * src, size_t count);


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
bool strcat_s(char * dest, size_t destsz, const char * src);



/**
 * @brief Change the extension on a pathname
 *
 * @param pathbuf The buffer in which to assemble the path
 * @param pathbuf_length The length of pathbuf, in bytes
 * @param filename The starting filename
 * @param extension The desired extension (c/w ".")
 *
 * @returns Returns zero on success, the needed buffer size on failure.
 */
bool change_extension(char * pathbuf, size_t pathbuf_length,
                      const char * filename, const char * extension);


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
char * hexlify(uint8_t * buf, size_t buflen, char * hexbuf, size_t hexbuflen);


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
void display_binary_data(uint8_t * blob, size_t length, bool show_all, char * indent);

#endif /* !_COMMON_UTIL_H */

