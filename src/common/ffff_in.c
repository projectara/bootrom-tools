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
 * @brief: This file contains the code reading a FFFF file.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include "ffff.h"
#include "ffff_common.h"
#include "ffff_in.h"
#include "ffff_print.h"


/*
 * Structure for caching element information during parsing.
 */
typedef struct
{
    const char *            filename;
    ffff_element_descriptor element;
} element_cache_entry;


/* Cache for element information from the command line. */
static element_cache_entry  element_cache[CALC_MAX_FFFF_ELEMENTS(FFFF_HEADER_SIZE_MAX)];
static uint32_t current_element = 0;
static uint32_t element_iterator = -1;
static bool     element_window_open = false;



/**
 * @brief Close the active cached element
 *
 * If "current_element" references to a "dirty" element, advance it to the
 * next element. Otherwise, leave it alone (close may be called multiple
 * times before the next open).
 *
 * @returns Nothng
 */
void element_cache_entry_close(void) {
    if (element_window_open) {
        /* Advance to the next element */
        if (current_element < _countof(element_cache)) {
            current_element++;
        }
    element_window_open = false;
    }
}


/**
 * @brief Open up an empty element cache entry and add the filename
 *
 * @param element_type The type of the element
 * @param filename The name of the file
 *
 * @returns 0 if successful, -1 otherwise
 */
int element_cache_entry_open(const uint32_t element_type,
                             const char * filename) {
    int linux_style_status = -1;  /* assume failure */
    element_cache_entry * element = NULL;

    if (current_element < _countof(element_cache)) {
        /**
         * If the args are such that we go directly from one element to the
         * next, we need to close the previous element before opening this
         * one
         */
        if (element_window_open) {
            element_cache_entry_close();
        }

       /*
         * Open the window and save the filename and type.
         * Note: we don't allocate and load the file  at this point.
         * We will load the file later.
         */
        element = &element_cache[current_element];
        element_window_open = true;
        element->element.element_type = element_type;
        if (filename != NULL) {
            element->filename = filename;
            element->element.element_length = size_file(filename);
        }

        linux_style_status = 0;
   }

    return linux_style_status;
}


/**
 * @brief Add the element class to the current element
 *
 * @param element_class The type of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_class(const uint32_t element_class) {
    bool success = false;  /* assume failure */


    if (element_window_open &&
        (current_element < _countof(element_cache))) {
        /* Open the window and save the filename and type */
        element_cache[current_element].element.element_class = element_class;
        success = true;
    } else {
        fprintf(stderr, "ERROR: no element to which to apply --eclass\n");
    }

    return success;
}


/**
 * @brief Add the element id to the current element
 *
 * @param element_id The type of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_id(const uint32_t element_id) {
    bool success = false;  /* assume failure */

    if (element_window_open &&
        (current_element < _countof(element_cache))) {
        /* Open the window and save the filename and type */
        element_cache[current_element].element.element_id = element_id;
        success = true;
    } else {
        fprintf(stderr, "ERROR: no element to which to apply --eid\n");
    }

    return success;
}


/**
 * @brief Add the element length to the current element
 *
 * @param element_length The length in bytes of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_length(const uint32_t element_length) {
    bool success = false;  /* assume failure */

    if (element_window_open &&
        (current_element < _countof(element_cache))) {
        /* Open the window and save the filename and type */
        element_cache[current_element].element.element_length = element_length;
        success = true;
    } else {
        fprintf(stderr, "ERROR: no element to which to apply --elen\n");
    }

    return success;
}


/**
 * @brief Add the element length to the active cached element
 *
 * @param element_length The type of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_location(const uint32_t element_location) {
    bool success = false;  /* assume failure */

    if (element_window_open &&
        (current_element < _countof(element_cache))) {
        /* Open the window and save the filename and type */
        element_cache[current_element].element.element_location = element_location;
        success = true;
    } else {
        fprintf(stderr, "ERROR: no element to which to apply --elen\n");
    }

    return success;
}


/**
 * @brief Add the element length to the active cached element
 *
 * @param element_length The type of the element
 *
 * @returns true if successful, false otherwise
 */
bool element_cache_entry_set_generation(const uint32_t element_generation) {
    bool success = false;  /* assume failure */

    if (element_window_open &&
        (current_element < _countof(element_cache))) {
        /* Open the window and save the filename and type */
        element_cache[current_element].element.element_generation = element_generation;
        success = true;
    } else {
        fprintf(stderr, "ERROR: no element to which to apply --elen\n");
    }

    return success;
}


/**
 * @brief Initialize the element cache iterator
 *
 * @returns true if successful, false otherwise
 */
void element_cache_init_iterator(void) {
    /* Mark the iterator to start at the beginning */
    element_iterator = -1;
}


/**
 * @brief Advance the iterator and size the data
 *
 * @param element_descriptor Where to store the element descriptor
 * @param total_size A pointer to a variable to which we add the sizes of
 *        the various elements.
 *
 * @returns 1 if successful, 0 if we reached the end of the list, or -1 if
 *          there was an error.
 */
int element_cache_get_next_entry_size(ffff_element_descriptor * element_descriptor,
                                 size_t * total_size) {
    int status = -1;  /* assume failure */
    bool success;
    uint8_t *payload_end;

    /* Next entry */
    element_iterator++;

    if ((element_iterator >= 0) &&
        (element_iterator < current_element)) {
        element_cache_entry * element = &element_cache[element_iterator];

        *total_size += element->element.element_length;
    }

    return status;
}


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
                              const uint32_t header_size) {
    size_t  total_size = 2 * header_size;
    ffff_element_descriptor * element;
    int status;
    int i = 0;
    bool content_will_fit = false;

    /* Determine the total header and payload length */
    element_cache_init_iterator();
    do {
        status = element_cache_get_next_entry_size(element, &total_size);
        i++;
    } while ((status > 0) &&
             (element->element_type != FFFF_ELEMENT_END) &&
             (i < ffff_max_elements));

    if (total_size > flash_capacity) {
        fprintf(stderr, "ERROR: %x-byte content exceeds %x-byte flash capacity\n",
                (uint32_t)total_size, flash_capacity);
    } else if (total_size > image_length) {
        fprintf(stderr, "ERROR: %x-byte content exceeds %x-byte FFFF image length\n",
                    (uint32_t)total_size, image_length);
    } else {
        content_will_fit = true;
    }

    return content_will_fit;
}


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
                                 uint8_t * bufend) {
    int status = -1;  /* assume failure */
    bool success;
    uint8_t *payload_end;

    /* Next entry */
    element_iterator++;

    if ((element_iterator >= 0) &&
        (element_iterator < current_element)) {
        element_cache_entry * element = &element_cache[element_iterator];

        /* Verify that the payload will fit in the payload buffer */
        payload_end = buffer +
                      element->element.element_length;
        if (payload_end > bufend) {
            fprintf(stderr,
                    "ERROR: element [%d]: 0x%x-byte payload will overrun remaining buffer (0x%x0bytes)\n",
                    element_iterator,
                    element->element.element_length,
                    (uint32_t)(bufend - buffer));
        } else {
            /* Copy the element descriptor */
            *element_descriptor = element->element; /* struct copy */

            /**
             *  Copy the element payload
             *
             * The parsing phase merely verified the file size - it is
             * now time to read it in.
             */
            success = load_file(element->filename,
                                &buffer[element->element.element_location],
                                element->element.element_length);

            if (success) {
                status = 1;
            }
        }
    } else {
        /* End of list */
        status = 0;
    }

    return status;
}


/**
 * @brief Return the number of cached elements
 *
 * @returns The number of cached elements
 */
uint32_t element_cache_entry_count(void) {
    return current_element;
}


/**
 * @brief Return the total size of all of the cached elements
 *
 * @returns The total number of element bytes
 */
ssize_t element_cache_entries_size(void) {
    uint32_t element;
    ssize_t total_size = 0;

    for (element = 0; element < current_element; element++) {
        total_size += element_cache[element].element.element_length;
    }
    return total_size;
}


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
                                      uint32_t image_length) {
    bool valid = true;
    int index;
    uint32_t lower_limit = 2 * next_boundary(header_size, erase_block_length);
    uint32_t element_location;

    for (index = 0; valid && index < current_element; index++) {
        element_location = element_cache[index].element.element_location;

        if (!block_aligned(element_location, erase_block_length)) {
            fprintf(stderr, "ERROR: Element is not block-aligned (%s)\n",
                    element_cache[index].filename);
            valid = false;
        } else if ((element_location < lower_limit) ||
                   (element_location >= image_length)) {
            fprintf(stderr,
                    "ERROR: Element location 0x%08x is out "
                    "of bounds (0x%08x..0x%08x) (%s)\n",
                    element_location, lower_limit, lower_limit + image_length,
                    element_cache[index].filename);
            valid = false;
        }
    }

    return valid;
}


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
                                const uint32_t header_size) {
    bool success = false;
    struct ffff * romimage = NULL;
    int status;


    romimage = new_ffff(image_length, header_size, erase_block_size);
    if (romimage != NULL) {
        ffff_header * ffff_hdr;
        uint32_t header_blob_length = next_boundary(header_size, erase_block_size);
        uint32_t tail_sentinel_offset = header_size - FFFF_SENTINEL_SIZE;
        ffff_element_descriptor * element;
        ffff_element_descriptor * last_element;

        /* Initialize the first FFFF header */
        ffff_hdr = romimage->ffff_hdrs[0];
        element = &ffff_hdr->elements[0];
        last_element = &ffff_hdr->elements[ffff_max_elements];
        memcpy(ffff_hdr->sentinel_value, ffff_sentinel_value, FFFF_SENTINEL_SIZE);
        ffff_set_timestamp(ffff_hdr);
        safer_strcpy(ffff_hdr->flash_image_name,
                     sizeof(ffff_hdr->flash_image_name),
                     name);
        if (strlen(ffff_hdr->flash_image_name) < strlen(name)) {
            fprintf(stderr, "Warning, flash_image_name truncated to '%s'\n",
                    ffff_hdr->flash_image_name);
        }
        ffff_hdr->flash_capacity = flash_capacity;
        ffff_hdr->erase_block_size = erase_block_size;
        ffff_hdr->header_size = header_size;
        ffff_hdr->flash_image_length = image_length;
        ffff_hdr->header_generation = generation;
        /**
         * Put an end-of-table marker at the start of the table. As we add
         * elements, we'll move the EOT element to the next position
         */
        element->element_type = FFFF_ELEMENT_END;


        /* Add the elements */
        element_cache_init_iterator();
        do {
            status =
                element_cache_get_next_entry(element,
                                             &romimage->blob[0],
                                             &romimage->blob[image_length]);
            if (element->element_type != FFFF_ELEMENT_END) {
                element++;
                /* Append the new end-of-table marker */
                element->element_type = FFFF_ELEMENT_END;
            }

        } while ((status > 0) && (element < last_element));
        if (status >= 0) {
            success = true;
        }

        /* Add the tail sentinel at the end of the header */
        memcpy(&romimage->blob[tail_sentinel_offset], ffff_sentinel_value,
               FFFF_SENTINEL_SIZE);

        /**
         * Now that the first FFFF header has been initialized, replicate
         * it into the 2nd FFFF header
         */
        memcpy(romimage->ffff_hdrs[1], romimage->ffff_hdrs[0],
               header_blob_length);

        if (!success) {
            romimage = free_ffff(romimage);
        }
    }

    return romimage;
}


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
struct ffff * read_ffff_romimage(const char * ffff_file) {
    ssize_t image_length;
    ssize_t bytes_read;
    int ffff_fd;
    struct ffff * romimage = NULL;
    ffff_header * ffff_hdr;
    uint32_t offset;
    bool success = false;

    image_length = size_file(ffff_file);
    if (image_length > (2 * FFFF_HEADER_SIZE_MIN)) {
        romimage = new_ffff((uint32_t)image_length, 0, 0);
        if (romimage != NULL) {

            /* Read the purported FFFF file */
            ffff_fd = open(ffff_file, O_RDONLY, 0666);
            if (ffff_fd < 0) {
                fprintf(stderr, "Error: unable to open '%s' (err %d)\n",
                        ffff_file,
                        ffff_fd);
            } else {
                bytes_read =
                    read(ffff_fd, romimage->blob, image_length);
                if (bytes_read != image_length) {
                    fprintf(stderr, "Error: Only read %u/%u bytes from '%s' (err %d)\n",
                            (uint32_t)bytes_read, (uint32_t)image_length,
                            ffff_file, errno);
                } else {
                    success = true;
                }

                /* Done with the FFFF file. */
                close(ffff_fd);
            }

            if (success) {
                ffff_hdr = romimage->ffff_hdrs[0];
                romimage->ffff_hdrs[1] = NULL; /* invalid until we find it */

                /**
                 * Examine the file for the FFFF headers
                 *
                 * According to the spec., the 1st FFFF header should be at offset
                 * 0, and the 2nd at the first 2**n boundary (at least as big as the
                 * minimum FFFF header size) past that. */
                if (validate_ffff_header(romimage->ffff_hdrs[0], 0) == 0) {
                    /* Found the 1st header, search for the 2nd */
                    ffff_header * ffff_hdr2 = NULL;

                    success = false;
                    for (offset = next_boundary(ffff_hdr->header_size,
                                                ffff_hdr->erase_block_size);
                         offset < image_length;
                         offset *= 2) {
                        ffff_hdr2 = (ffff_header *)&romimage->blob[offset];
                        if (validate_ffff_header(ffff_hdr2, 0) == 0) {
                            romimage->ffff_hdrs[1] = ffff_hdr2;
                            success = true;
                            break;
                        }
                    }
                } else {
                    success = false;
                    /* No valid header at location 0, search for the 2nd */
                    for (offset = next_boundary(FFFF_HEADER_SIZE_MIN,
                                                FFFF_HEADER_SIZE_MIN);
                         offset < image_length;
                         offset *= 2) {
                        ffff_hdr = (ffff_header *)&romimage->blob[offset];
                        if (validate_ffff_header(ffff_hdr, offset) == 0) {
                            romimage->ffff_hdrs[0] = ffff_hdr;
                            success = true;
                            break;
                        }
                    }
                }
             }
        }
    }

    if (success) {
        return romimage;
    } else {
        return NULL;
    }
}


/*-----
 * The following is scrounged from <bootrom>/common/src/ffff.c
 * TODO: REFACTOR s.t. both tools use the same code from 1 source
 */

#ifdef BOOTROM
    /* Bootrom environment */
#else
    /* Bootrom-tools environment */
    #define set_last_error(x)    /* Suppress the bootloader error logging */
#endif /* BOOTROM */


/**
 * @brief Determine if the FFFF element type is valid
 *
 * @param element_type The element type to check
 *
 * @returns True if a valid (i.e., known) element type, false otherwise
 */
bool valid_ffff_type(uint32_t element_type) {
     return (((element_type >= FFFF_ELEMENT_STAGE_2_FW) &&
              (element_type <= FFFF_ELEMENT_DATA)) ||
             (element_type == FFFF_ELEMENT_END));
}


/**
 * @brief Validate an FFFF element
 *
 * @param section The FFFF element descriptor to validate
 * @param header (The RAM-address of) the FFFF header to which it belongs
 * @param rom_address The address in SPIROM of the header
 * @param end_of_elements Pointer to a flag that will be set if the element
 *        type is the FFFF_ELEMENT_END marker. (Untouched if not)
 *
 * @returns True if valid element, false otherwise
 */
bool valid_ffff_element(ffff_element_descriptor * element,
                        ffff_header * header, uint32_t rom_address,
                        bool *end_of_elements) {
    ffff_element_descriptor * other_element;
    uint32_t element_location_min = rom_address + header->erase_block_size;
    uint32_t element_location_max = header->flash_image_length;
    uint32_t this_start;
    uint32_t this_end;
    uint32_t that_start;
    uint32_t that_end;

    /*
     * Because we don't *know* the real length of the Flash, it is possible to
     * read past the end of the flash (which simply wraps around). This is benign
     * because the resulting mess will not validate.
     */

    /* Is this the end-of-table marker? */
    if (element->element_type == FFFF_ELEMENT_END) {
        *end_of_elements = true;
        return true;
    }

    /* Do we overlap the header or spill over the end? */
    this_start = element->element_location;
    this_end = this_start + element->element_length - 1;
    if ((this_start < element_location_min) ||
        (this_end >= element_location_max)) {
        set_last_error(BRE_FFFF_ELT_RESERVED_MEMORY);
        return false;
    }

    /* Are we block-aligned? */
    if (!block_aligned(element->element_location, header->erase_block_size)) {
        set_last_error(BRE_FFFF_ELT_ALIGNMENT);
        return false;
    }

    /*
     * Check this element for:
     *   a) collisions against all following sections and
     *   b) duplicates of this element.
     * Since we're called in a scanning fashion from the start to the end of
     * the elements array, all elements before us have already validated that
     * they don't duplicate or collide with us.
     */
    for (other_element = element + 1;
         ((other_element < &header->elements[ffff_max_elements]) &&
          (other_element->element_type != FFFF_ELEMENT_END));
         other_element++) {
        /* (a) check for collision */
        that_start = other_element->element_location;
        that_end = that_start + other_element->element_length - 1;
        if ((that_end >= this_start) && (that_start <= this_end)) {
            set_last_error(BRE_FFFF_ELT_COLLISION);
            return false;
        }


        /*
         * (b) Check for  duplicate entries per the specification:
         * "At most, one element table entry with a particular element type,
         * element ID, and element generation may be present in the element
         * table."
         */
        if ((element->element_type == other_element->element_type) &&
            (element->element_id == other_element->element_id) &&
            (element->element_generation ==
                    other_element->element_generation)) {
            set_last_error(BRE_FFFF_ELT_DUPLICATE);
            return false;
        }
    }

    /* No errors found! */
    return true;
}


/**
 * @brief Validate an FFFF header
 *
 * @param header The FFFF header to validate
 * @param address The address in SPIROM of the header
 *
 * @returns 0 if valid element, -1 otherwise
 */
int validate_ffff_header(ffff_header *header, uint32_t address) {
    int i;
    ffff_element_descriptor * element;
    bool end_of_elements = false;
    char * tail_sentinel = ((char*)(header)) + header->header_size -
                            FFFF_SENTINEL_SIZE;


    /* Check for leading and trailing sentinels */
    for (i = 0; i < FFFF_SENTINEL_SIZE; i++) {
        if (header->sentinel_value[i] != ffff_sentinel_value[i]) {
            set_last_error(BRE_FFFF_SENTINEL);
            return -1;
        }
    }
    for (i = 0; i < FFFF_SENTINEL_SIZE; i++) {
        if (tail_sentinel[i] != ffff_sentinel_value[i]) {
            set_last_error(BRE_FFFF_SENTINEL);
            return -1;
        }
    }

    if (header->erase_block_size > FFFF_ERASE_BLOCK_SIZE_MAX) {
        set_last_error(BRE_FFFF_BLOCK_SIZE);
        return -1;
    }

    if (header->flash_capacity < (header->erase_block_size << 1)) {
        set_last_error(BRE_FFFF_FLASH_CAPACITY);
        return -1;
    }

    if (header->flash_image_length > header->flash_capacity) {
        set_last_error(BRE_FFFF_IMAGE_LENGTH);
        return -1;
    }

    if ((header->header_size < FFFF_HEADER_SIZE_MIN) ||
        (header->header_size > FFFF_HEADER_SIZE_MAX)) {
        set_last_error(BRE_FFFF_HEADER_SIZE);
       return -1;
    }

    /* Validate the FFFF elements */
    for (element = &header->elements[0];
         (element < &header->elements[ffff_max_elements]) && !end_of_elements;
         element++) {
        if (!valid_ffff_element(element, header, address, &end_of_elements)) {
            /* (valid_ffff_element took care of error reporting) */
            return -1;
        }
    }
    if (!end_of_elements) {
        set_last_error(BRE_FFFF_NO_TABLE_END);
        return -1;
    }

    /*
     * Verify that the remainder of the header (i.e., unused section
     * descriptors and the padding) is zero-filled
     */
    if (!is_constant_fill((uint8_t *)element,
                          (uint32_t)((tail_sentinel) - (char *)element),
                          0x00)) {
        set_last_error(BRE_FFFF_NON_ZERO_PAD);
        return -1;
    }

    /* No errors found! */
    return 0;
}
