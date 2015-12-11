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
 * @brief: This file contains the code for reading a FFFF file.
 *
 */

#ifndef _COMMON_FFFF_IN_H
#define _COMMON_FFFF_IN_H

#include "ffff_common.h"


/**
 * @brief Close the active cached element
 *
 * If "current_element" references to a "dirty" element, advance it to the
 * next element. Otherwise, leave it alone (close may be called multiple
 * times before the next open).
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
void element_cache_entry_close(void);



/**
 * @brief Open up an empty element cache entry and add the filename
 *
 * @param element_type The type of the element
 * @param filename The name of the file
 *
 * @returns 0 if successful, -1 otherwise
 */
int element_cache_entry_open(const uint32_t element_type,
                              const char * filename);


/**
 * @brief Add the element class to the current element
 *
 * @param element_class The type of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_class(const uint32_t element_class);


/**
 * @brief Add the element id to the active cached element
 *
 * @param element_id The type of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_id(const uint32_t element_id);


/**
 * @brief Add the element length to the active cached element
 *
 * @param element_length The type of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_length(const uint32_t element_length);


/**
 * @brief Add the element length to the active cached element
 *
 * @param element_length The type of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_location(const uint32_t element_location);


/**
 * @brief Add the element length to the active cached element
 *
 * @param element_length The type of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_generation(const uint32_t element_generation);


/**
 * @brief Initialize the element cache iterator
 *
 * @returns true if successful, false otherwise
 */
void element_cache_init_iterator(void);


/**
 * @brief Advance the iterator and extract the data
 *
 * @param element_descriptor Where to store the element descriptor
 * @param buffer A pointer to the image buffer.
 * @param bufend A pointer to the end of the buffer, to prevent
 *              overruns.
 *
 * @returns 1 if successful, 0 if we reached the end of the list, or -1 if
 *          there was an error.
 */
int element_cache_get_next_entry(ffff_element_descriptor * element_descriptor,
                                 uint8_t * buffer,
                                 uint8_t * bufend);


/**
 * @brief Close the active cached element
 *
 * If "current_element" points to a "dirty" element, advance it to the
 * next element. Otherwise, leave it alone.
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
void element_cache_entry_close(void);


/**
 * @brief Return the number of cached elements
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
uint32_t element_cache_entry_count(void);


/**
 * @brief Return the total size of all of the cached elements
 *
 * @returns The total number of element bytes
 */
ssize_t element_cache_entries_size(void);
/**
 * @brief Return the number of cached elements
 *
 * @param header_size The size in bytes of the header
 * @image erase_block_length The Flash erase block size
 * @param image_length The length of the FFFF
 *
 * @returns True if the element locations seem valid, false otherwise
 */
bool element_cache_validate_locations(uint32_t header_size,
                                      uint32_t erase_block_length,
                                      uint32_t image_length);


/**
 * @brief Determine if the content will fit in the FFFF romimage
 *
 * @param flash_capacity The capacity of the Flash, in bytes
 * @param image_length The length of the FFFF
 * @param header_size The size in bytes of the header
 *
 * @returns True if the content will fit in the image_length, false
 *          otherwise
 */
bool check_ffff_romimage_size(const uint32_t flash_capacity,
                              const uint32_t image_length,
                              const uint32_t header_size);


/**
 * @brief Allocate and initialize a FFFF romimage, including elements
 *
 * This allocates a blob for an FFFF, initializes the header and element
 * descriptor table, and loads all of the element payloads.
 *
 * @param name The tname of the FFFF
 * @param flash_capacity The capacity of the Flash, in bytes
 * @image erase_block_size The Flash erase block size
 * @param image_length The length of the FFFF
 * @param generation The FFFF generation number (used in image update)
 * @param header_size The size in bytes of the header
 *
 * @returns If successful, returns a pointer to an initialized romimage,
 *          NULL otherwise
 */
struct ffff * new_ffff_romimage(const char *   name,
                                const uint32_t flash_capacity,
                                const uint32_t erase_block_size,
                                const uint32_t image_length,
                                const uint32_t generation,
                                const uint32_t header_size);


/**
 * @brief Allocate and initialize a FFFF romimage from an FFFF file
 *
 * This allocates a blob for an FFFF, initializes the header and element
 * descriptor table, and loads all of the element payloads.
 *
 * @param ffff_file The name of the FFFF file
 *
 * @returns If successful, returns a pointer to an initialized romimage,
 *          NULL otherwise
 */
struct ffff * read_ffff_romimage(const char * ffff_file);

/**
 * @brief Validate an FFFF header
 *
 * @param header The FFFF header to validate
 * @param address The address in SPIROM of the header
 *
 * @returns 0 if valid element, -1 otherwise
 */
int validate_ffff_header(ffff_header *header, uint32_t address);

#endif /* _COMMON_FFFF_IN_H */

