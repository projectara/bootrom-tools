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
 * @brief: This file contains the code for reading a TFTF file.
 *
 */

#ifndef _COMMON_TFTF_IN_H
#define _COMMON_TFTF_IN_H



/**
 * @brief Determine the size of a file
 *
 * @param filename The name of the file to check
 *
 * @returns Returns On success, returns the length of the file in bytes;
 *          -1 on failure
 */
ssize_t size_file(const char * filename);


/**
 * @brief Read a file into a buffer
 *
 * @param filename The name of the file to load into buf
 * @param buf A pointer to the buffer in which to load the file
 * @param length The length of the buffer
 *
 * @returns Returns true on success, false on failure
 */
bool load_file(const char * filename, uint8_t * buf, size_t length);


/**
 * @brief Allocate a buffer and read a file into it
 *
 * @param filename The name of the (TFTF section) file to append to
 * the TFTF output file
 * @param length Pointer to a value to hold the length of the blob
 *
 * @returns Returns a pointer to an allocated buf containing the file contents
 *          on success, NULL on failure
 */
uint8_t * alloc_load_file(const char * filename, ssize_t * length);



/**
 * @brief Open up an empty section cache entry and add the filename
 *
 * @param section_type The type of the section
 * @param filename The name of the file
 *
 * @returns 0 if successful, -1 otherwise
 */
int section_cache_entry_open(const uint32_t section_type,
                              const char * filename);


/**
 * @brief Add the section class to the current section
 *
 * @param section_class The type of the section
 *
 * @returns true if successful, false otherwise
 */
bool section_cache_entry_set_class(const uint32_t section_class);


/**
 * @brief Add the section id to the active cached section
 *
 * @param section_id The type of the section
 *
 * @returns true if successful, false otherwise
 */
bool section_cache_entry_set_id(const uint32_t section_id);


/**
 * @brief Add the section load address to the active cached section
 *
 * @param section_class The type of the section
 *
 * @returns true if successful, false otherwise
 */
bool section_cache_entry_set_load_address(const uint32_t load_address);


/**
 * @brief Initialize the section cache iterator
 *
 * @returns true if successful, false otherwise
 */
void section_cache_init_iterator(void);


/**
 * @brief Advance the iterator and extract the data
 *
 * @param section_descriptor Where to store the section descriptor
 * @param ppayload A pointer to the pointer to the payload buffer. (It
 *                 advances the caller's pointer to the first byte past the
 *                 copied payload
 * @param limit A pointer to the end of the payload buffer, to prevent
 *              overruns.
 *
 * @returns 1 if successful, 0 if we reached the end of the list, or -1 if
 *          there was an error.
 */
int section_cache_get_next_entry(tftf_section_descriptor * section_descriptor,
                                 uint8_t ** ppayload,
                                 uint8_t * limit);


/**
 * @brief Close the active cached section
 *
 * If "current_section" points to a "dirty" section, advance it to the
 * next section. Otherwise, leave it alone.
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
void section_cache_entry_close(void);


/**
 * @brief Return the number of cached sections
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
uint32_t section_cache_entry_count(void);


/**
 * @brief Return the total size of all of the cached sections
 *
 * @returns The total number of section bytes
 */
ssize_t section_cache_entries_size(void);


/**
 * @brief Allocate and initialize a TFTF blob
 *
 * @param header The size in bytes of the header
 * @param payload_size The total size of all section payloads
 * @param firmware_pkg_name The name of the TFTF
 * @param start_location The starting address (TFTF entry point)
 * @param unipro_md The UniPro Manufacturer's ID
 * @param unipro_pid The UniPro Product ID
 * @param ara_vid The ARA vendor ID
 * @param ara_pid The ARA product ID
 *
 * @returns If successful, returns a pointer to a blob with an
 *          initialized TFTF header at the beginning; NULL
 *          otherwise.
 */
tftf_header * new_tftf(const uint32_t header_size,
                       const uint32_t payload_size,
                       const char *   firmware_pkg_name,
                       const uint32_t package_type,
                       const uint32_t start_location,
                       const uint32_t unipro_mid,
                       const uint32_t unipro_pid,
                       const uint32_t ara_vid,
                       const uint32_t ara_pid);


/**
 * @brief Parse a .elf file into the section cache
 *
 * Parse a .elf file and extract the text (code) and data segments, creating
 * code and data segments in the cache.
 *
 * @param filename The pathanme of the .elf file
 * @param start_address Pointer to the TFTF start_address field. This will
 *        be set to the .elf file entrypoint (e_entry) if not already set
 *        by the "--start" parameter.
 *
 * @returns True if successful, false otherwise.
 */
bool load_elf(const char * filename, uint32_t * start_address);


/**
 * @brief Validate the TFTF header.
 *
 * Check the TFTF header for overlapped regions.
 *
 * @param tftf_header The TFTF header to check
 *
 * @returns Returns true on success, false on failure.
 */
bool validate_tftf_header(tftf_header * tftf_hdr);

#endif /* _COMMON_TFTF_IN_H */

