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
 * @brief: This file contains the code reading a TFTF file.
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
#include <gelf.h>
#include "util.h"
#include "tftf.h"
#include "tftf_common.h"
#include "tftf_in.h"


/*
 * Structure for caching section information during parsing. The blob is
 * used by the --elf handler, which typically generates multiple sections
 * from one file - we cache the data/code chunk of the elf file at parse
 * time.  At post-parse time, we allocate a buffer for the whole tftf
 * header and payload, and read the files and copy the blobs into it.
 */
typedef struct
{
    const char *            filename;
    uint8_t *               blob;
    tftf_section_descriptor section;
} section_cache_entry;


/* Cache for section information from the command line. */
#define MAX_TFTF_SECTION_CACHE	256  /* arbitrary # */
static section_cache_entry   section_cache[MAX_TFTF_SECTION_CACHE];
static uint32_t current_section = 0;
static uint32_t section_iterator = -1;
static bool     section_window_open = false;
static uint32_t section_load_address = 0;


/**
 * @brief Close the active cached section
 *
 * If "current_section" references to a "dirty" section, advance it to the
 * next section. Otherwise, leave it alone (close may be called multiple
 * times before the next open).
 *
 * @returns 0 on success, 1 if there were warnings, 2 on failure
 */
void section_cache_entry_close(void) {
    if (section_window_open) {
        uint32_t    this_section_address;
        /**
         * Section load_addresses are normally contiguous, so adjust the
         * running section_load_address to the end of the current section.
         * The next section will pick this up as the default when opened by
         * section_cache_entry_open().
         */
        section_load_address =
                section_cache[current_section].section.section_load_address +
                section_cache[current_section].section.section_length;

        /* Advance to the next section */
        if (current_section < MAX_TFTF_SECTION_CACHE) {
            current_section++;
        }
    section_window_open = false;
    }
}


/**
 * @brief Open up an empty section cache entry and add the filename
 *
 * @param section_type The type of the section
 * @param filename The name of the file
 *
 * @returns 0 if successful, -1 otherwise
 */
int section_cache_entry_open(const uint32_t section_type,
                              const char * filename) {
    int linux_style_status = -1;  /* assume failure */
    section_cache_entry * section = NULL;

    if (current_section < MAX_TFTF_SECTION_CACHE) {
        /**
         * If the args are such that we go directly from one section to the
         * next, we need to close the previous section before opening this
         * one
         */
        if (section_window_open) {
            section_cache_entry_close();
        }

        /*
         * Open the window and save the filename and type.
         * Note: we don't allocate and load the file into
         * section_cache[current_section].blob at this point. We will load
         * the file later.
         */
        section = &section_cache[current_section];
        section_window_open = true;
        section->blob = NULL;
        section->section.section_type = section_type;
        if (filename != NULL) {
            section->filename = filename;
            section->section.section_length =
                section->section.section_expanded_length =
                    size_file(filename);
        }

        /* By default, each section's load address is contiguous with
         * the previous section. Set the default load address here - we
         * will override it in section_cache_entry_set_load_address, and
         * section_cache_entry_close will advance the running
         * section_load_address to the section's load_address + load_length.
         */
        section->section.section_load_address = section_load_address;
        linux_style_status = 0;
   }

    return linux_style_status;
}


/**
 * @brief Add the section class to the current section
 *
 * @param section_class The type of the section
 *
 * @returns true if successful, false otherwise
 */
bool section_cache_entry_set_class(const uint32_t section_class) {
    bool success = false;  /* assume failure */


    if (section_window_open &&
        (current_section < MAX_TFTF_SECTION_CACHE)) {
        /* Open the window and save the filename and type */
        section_cache[current_section].section.section_class = section_class;
        success = true;
    } else {
        fprintf(stderr, "ERROR: no section to which to apply --class\n");
    }

    return success;
}


/**
 * @brief Add the section id to the current section
 *
 * @param section_id The type of the section
 *
 * @returns true if successful, false otherwise
 */
bool section_cache_entry_set_id(const uint32_t section_id) {
    bool success = false;  /* assume failure */

    if (section_window_open &&
        (current_section < MAX_TFTF_SECTION_CACHE)) {
        /* Open the window and save the filename and type */
        section_cache[current_section].section.section_id = section_id;
        success = true;
    } else {
        fprintf(stderr, "ERROR: no section to which to apply --id\n");
    }

    return success;
}


/**
 * @brief Add the section load address to the current section
 *
 * @param section_class The type of the section
 *
 * @returns true if successful, false otherwise
 */
bool section_cache_entry_set_load_address(const uint32_t load_address) {
    bool success = false;  /* assume failure */

    if (section_window_open &&
        (current_section < MAX_TFTF_SECTION_CACHE)) {
        /* Open the window and save the filename and type */
        section_cache[current_section].section.section_load_address = load_address;
        success = true;
    } else {
        fprintf(stderr, "ERROR: no section to which to apply --load\n");
    }

    return success;
}


/**
 * @brief Add a data blob to the current section
 *
 * @param section_class The type of the section
 *
 * @returns true if successful, false otherwise
 */
bool section_cache_entry_set_blob(const uint8_t *blob, const size_t blob_length) {
    bool success = false;  /* assume failure */

    if (section_window_open &&
        (current_section < MAX_TFTF_SECTION_CACHE)) {
        /* Open the window and save the filename and type */
        section_cache[current_section].blob = malloc(blob_length);
        if (section_cache[current_section].blob == NULL) {
            fprintf(stderr, "ERROR: Can't allocate section blob\n");
        } else {
            memcpy(section_cache[current_section].blob, blob, blob_length);
            section_cache[current_section].section.section_expanded_length =
            section_cache[current_section].section.section_length =
                blob_length;
            success = true;
        }
    } else {
        fprintf(stderr, "ERROR: no section to which to set a data blob\n");
    }

    return success;
}


/**
 * @brief Initialize the section cache iterator
 *
 * @returns true if successful, false otherwise
 */
void section_cache_init_iterator(void) {
    /* Mark the iterator to start at the beginnig */
    section_iterator = -1;
}


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
                                 uint8_t * limit) {
    int status = -1;  /* assume failure */
    bool success;
    uint8_t *payload_end;

    /* Next entry */
    section_iterator++;

    if ((section_iterator >= 0) &&
        (section_iterator < current_section)) {
        section_cache_entry * section = &section_cache[section_iterator];

        /* Verify that the payload will fit in the payload buffer */
        payload_end = *ppayload +
                      section->section.section_expanded_length;
        if (payload_end > limit) {
            fprintf(stderr, "ERROR: section payload will overrun buffer\n");
        } else {
            /* Copy the section descriptor */
            *section_descriptor = section->section; /* struct copy */

            /* Copy the section payload */
            if (section->blob != NULL) {
                /* We have payload from the parsing phase */
                memcpy(*ppayload, section->blob,
                       section->section.section_expanded_length);
                success = true;
            } else {
                /*
                 * The parsing phase merely verified the file size - it is
                 * now time to read it in.
                 */
                success =
                    load_file(section->filename,
                              *ppayload,
                              section->section.section_expanded_length);
            }
            if (success) {
                *ppayload = payload_end;
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
 * @brief Return the number of cached sections
 *
 * @returns The number of cached sections
 */
uint32_t section_cache_entry_count(void) {
    return current_section;
}


/**
 * @brief Return the total size of all of the cached sections
 *
 * @returns The total number of section bytes
 */
ssize_t section_cache_entries_size(void) {
    uint32_t section;
    size_t total_size = 0;

    for (section = 0; section < current_section; section++) {
        total_size += section_cache[section].section.section_expanded_length;
    }
    return total_size;
}


/**
 * @brief Allocate and initialize a TFTF blob, including sections
 *
 * This allocates a blob for a TFTF, initializes the header and section
 * descriptor table, and loads all of the section payloads.
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
                       const uint32_t ara_pid) {
    bool success = false;
    tftf_header * tftf_hdr = NULL;
    uint32_t blob_length = header_size + payload_size;

    tftf_hdr = new_tftf_blob(header_size, payload_size);
    if (tftf_hdr != NULL) {
        tftf_section_descriptor *section = tftf_hdr->sections;
        uint8_t * hdr_end = ((uint8_t *)tftf_hdr) + tftf_hdr->header_size;
        uint8_t * payload = (uint8_t *)tftf_hdr + header_size;
        uint8_t * limit = (uint8_t *)tftf_hdr + blob_length;
        size_t fw_pkg_name_length = sizeof(tftf_hdr->firmware_package_name);
        int status;

        /* Initialize the remaining fixed parts of the header */
        set_timestamp(tftf_hdr);
        if (firmware_pkg_name != NULL) {
            /* Copy the string, ensuring the buffer is ASCIIZ */
            safer_strncpy(tftf_hdr->firmware_package_name, fw_pkg_name_length,
                      firmware_pkg_name, fw_pkg_name_length);
            if (strlen(tftf_hdr->firmware_package_name) < strlen(firmware_pkg_name)) {
                fprintf(stderr,
                        "Warning: firmware package name has been truncated to '%s'\n",
                        tftf_hdr->firmware_package_name);
            }
        }
        tftf_hdr->package_type = package_type;
        tftf_hdr->start_location = start_location;
        tftf_hdr->unipro_mid = unipro_mid;
        tftf_hdr->unipro_pid = unipro_pid;
        tftf_hdr->ara_vid = ara_vid;
        tftf_hdr->ara_pid = ara_pid;

        /* Add the sections */
        section_cache_init_iterator();
        do {
            status = section_cache_get_next_entry(section, &payload, limit);
            section++;
        } while ((status > 0) && (payload < limit) &&
                 (section < (tftf_section_descriptor *)hdr_end));
        if (status >= 0) {
            /* Add the end-of-table marker */
            section->section_type = TFTF_SECTION_END;
            success = true;
        }


        if (!success) {
            tftf_hdr = free_tftf_header(tftf_hdr);
        }
    }

    return tftf_hdr;
}


/**
 * @brief Search an elf for a named section
 *
 * @param elf The open elf object
 * @param name The section name to find
 *
 * @returns A pointer to the section if successful, NULL otherwise.
 */
static Elf_Scn * elf_getscn_byname(Elf *elf, const char * name) {
    Elf_Scn *scn = NULL;
    size_t shstrndx;
    GElf_Shdr shdr;
    char *section_name;

    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        fprintf(stderr, "elf_getshdrstrndx() failed: %s\n", elf_errmsg(-1));
    } else {
        /**
         * Iterate over the sections in the ELF descriptor to get the indices
         * of the .text and .data descriptors
         */
        while ((scn = elf_nextscn(elf, scn)) != NULL) {
            if (gelf_getshdr(scn, &shdr) != &shdr) {
                fprintf(stderr, "getshdr() failed: %s\n",
                    elf_errmsg(-1));
            } else {
                section_name = elf_strptr(elf, shstrndx, shdr.sh_name);
                if (section_name == NULL) {
                    fprintf(stderr, "elf_strptr() failed: %s\n",
                        elf_errmsg(-1));
                } else if (strcmp(section_name, name) == 0) {
                    break;
                }
            }
        }
    }

    return scn;
}


/**
 * @brief Add an ELF section to the TFTF section cache.
 *
 * @param scn The open elf section (e.g., .text, .data)
 * @param type The corresponding section_type for the cache
 *
 * @returns A pointer to the section if successful, NULL otherwise.
 */
bool elf_add_section_cache_entry(Elf_Scn *scn, uint32_t type) {
    bool success = false;
    GElf_Shdr shdr;
    Elf_Data *data = NULL;
    size_t n = 0;

    /* Get the associated ELF Section Header */
    if (gelf_getshdr(scn, &shdr) != &shdr) {
        fprintf(stderr, "getshdr(shstrndx) failed: %s\n",elf_errmsg(-1));
    } else {
        while ((n < shdr.sh_size) &&
               (data = elf_getdata(scn, data)) != NULL) {
            success = (section_cache_entry_open(type, NULL) == 0) &&
                      section_cache_entry_set_blob(data->d_buf, data->d_size);
            section_cache_entry_close();
            break;
        }
    }

    return success;
}


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
bool load_elf(const char * filename, uint32_t * start_address) {
    bool success = false;
    int fd = -1;
    Elf *elf = NULL;
    Elf_Scn *scn = NULL;
    size_t shstrndx;
    int sections_created = 0;

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library initialization failed: %s\n",
                elf_errmsg(-1));
        goto cleanup;
    }

    if ((fd = open(filename, O_RDONLY, 0)) < 0) {
        fprintf(stderr, "ERROR: Can't open '%s' (err %d)\n", filename, errno);
        goto cleanup;
    }

    if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
        fprintf(stderr, "elf_begin() failed: %s\n", elf_errmsg(-1));
        goto cleanup;
    }

    if (elf_kind(elf) != ELF_K_ELF) {
        fprintf(stderr,  "%s is not an ELF object\n", filename);
        goto cleanup;
    }

    if (elf_getshdrstrndx(elf, &shstrndx) != 0) {
        fprintf(stderr, "elf_getshdrstrndx() failed: %s\n", elf_errmsg(-1));
        goto cleanup;
    }

    /**
     * Extract the .text and .data segments and add them to the section cache
     */
    scn = elf_getscn_byname(elf, ".text");
    if (scn != NULL) {
        success = elf_add_section_cache_entry(scn, TFTF_SECTION_RAW_CODE);
        if (success) {
            /* Extract entrypoint (e_entry) */
            Elf32_Ehdr * ehdr = NULL;

            ehdr = elf32_getehdr(elf);
            if (ehdr != NULL) {
                if (start_address && *start_address == 0) {
                    *start_address = (uint32_t)ehdr->e_entry;
                }
            } else {
                fprintf(stderr, "ERROR: elf32_getehdr failed\n");
            }

            sections_created++;
        }
    }

    scn = elf_getscn_byname(elf, ".data");
    if (scn != NULL) {
        success = elf_add_section_cache_entry(scn, TFTF_SECTION_RAW_DATA);
        if (success) {
            sections_created++;
        }
    }

    if (sections_created >= 1) {
        success = true;
    } else {
        fprintf(stderr, "ERROR: no code or data segments in '%s'\n", filename);
        success = false;
    }


cleanup:
    if (elf != NULL) {
        elf_end(elf);
    }
    if (fd != -1) {
        close(fd);
    }
    return success;
}

/*-----
 * The following is scrounged from <bootrom>/common/src/tft.c
 * TODO: REFACTOR s.t. both tools use the same code from 1 source
 */

#ifdef BOOTROM
    /* Bootrom environment */
#else
    /* Bootrom-tools environment */
    #define set_last_error(x)    /* Suppress the bootloader error logging */
#endif /* BOOTROM */

/**
 * @brief Determine if the TFTF section type is valid
 *
 * @param section_type The section type to check
 *
 * @returns True if a valid section type, false otherwise
 */
bool valid_tftf_type(uint32_t section_type) {
     return (((section_type >= TFTF_SECTION_RAW_CODE) &&
              (section_type <= TFTF_SECTION_MANIFEST)) ||
             (section_type == TFTF_SECTION_SIGNATURE) ||
             (section_type == TFTF_SECTION_CERTIFICATE) ||
             (section_type == TFTF_SECTION_END));
}

/**
 * @brief Validate a TFTF section descriptor
 *
 * @param section The TFTF section descriptor to validate
 * @param header The TFTF header to which it belongs
 * @param section_contains_start Pointer to a flag that will be set if the
 *        image entry point lies within this section. (Untouched if not)
 * @param end_of_sections Pointer to a flag that will be set if the section
 *        type is the end-of-section-table marker. (Untouched if not)
 *
 * @returns True if valid section, false otherwise
 */
bool valid_tftf_section(tftf_section_descriptor * section,
                        tftf_header * header,
                        bool * section_contains_start,
                        bool * end_of_sections) {
    uint32_t    section_start;
    uint32_t    section_end;
    uint32_t    other_section_start;
    uint32_t    other_section_end;
    tftf_section_descriptor * other_section;

    if (!valid_tftf_type(section->section_type)) {
        set_last_error(BRE_TFTF_HEADER_TYPE);
        return false;
    }

    /* Is this the end-of-table marker? */
    if (section->section_type == TFTF_SECTION_END) {
        *end_of_sections = true;
        return true;
    }

    /*
     * Convert the section limits to absolute addresses to compare against
     * absolute addresses found in the TFTF header.
     */
    section_start = section->section_load_address;
    section_end = section_start + section->section_expanded_length;

    if (section_start == DATA_ADDRESS_TO_BE_IGNORED) {
        return true;
    }

    /* Verify the expanded/compressed lengths are sane */
    if (section->section_expanded_length < section->section_length) {
        set_last_error(BRE_TFTF_COMPRESSION_BAD);
        return false;
    }

#ifdef BOOTROM
    /* can this section fit into the system memory */
    if (chip_validate_data_load_location((void *)section_start,
                                         section->section_expanded_length)) {
        set_last_error(BRE_TFTF_MEMORY_RANGE);
        return false;
    }
#endif

    /* Does the section contain the entry point? */
    if ((header->start_location >= section_start) &&
        (header->start_location < section_end) &&
        (section->section_type == TFTF_SECTION_RAW_CODE)) {
        *section_contains_start = true;
    }

    /*
     * Check this section for collision against all following sections.
     * Since we're called in a scanning fashion from the start to the end of
     * the sections array, all sections before us have already validated that
     * they don't collide with us.
     *
     * Overlap is determined to be "non-disjoint" sections
     */
    for (other_section = section + 1;
         ((other_section < &header->sections[tftf_max_sections]) &&
          (other_section->section_type != TFTF_SECTION_END) &&
          (other_section->section_load_address != DATA_ADDRESS_TO_BE_IGNORED));
         other_section++) {
        other_section_start = other_section->section_load_address;
        other_section_end = other_section_start +
                            other_section->section_expanded_length;
        if ((other_section->section_type != TFTF_SECTION_END) &&
            (!((other_section_end < section_start) ||
            (other_section_start >= section_end)))) {
            set_last_error(BRE_TFTF_COLLISION);
            return false;
        }
    }

    return true;
}

/**
 * @brief Validate a TFTF header
 *
 * @param header The TFTF header to validate
 *
 * @returns True if valid TFTF header, false otherwise
 */
bool valid_tftf_header(tftf_header * header) {
    tftf_section_descriptor * section;
    bool section_contains_start = false;
    bool end_of_sections = false;
    uint8_t * end_of_header;
    int i;

    /* Verify the sentinel */
    for (i = 0; i < TFTF_SENTINEL_SIZE; i++) {
        if (header->sentinel_value[i] != tftf_sentinel[i]) {
            set_last_error(BRE_TFTF_SENTINEL);
            return false;
        }
    }

    if (header->header_size != TFTF_HEADER_SIZE) {
        set_last_error(BRE_TFTF_HEADER_SIZE);
        return false;
    }

    /* Verify all of the sections */
    for (section = &header->sections[0];
         (section < &header->sections[tftf_max_sections]) && !end_of_sections;
         section++) {
        if (!valid_tftf_section(section, header, &section_contains_start,
                                &end_of_sections)) {
            /* (valid_tftf_section took care of error reporting) */
            return false;
        }
    }
    if (!end_of_sections) {
        set_last_error(BRE_TFTF_NO_TABLE_END);
        return false;
    }

    /* Verify that, if this TFTF has a start address, it falls in one of our code sections. */
    if ((header->start_location != 0) && !section_contains_start) {
        set_last_error(BRE_TFTF_START_NOT_IN_CODE);
        return false;
    }

    /*
     * Verify that the remainder of the header (i.e., unused section
     * descriptors and the padding) is zero-filled
     */
    end_of_header = (uint8_t *)(header);
    end_of_header += header->header_size;
    if (!is_constant_fill((uint8_t *)section,
                          end_of_header - (uint8_t *)section,
                          0x00)) {
        set_last_error(BRE_TFTF_NON_ZERO_PAD);
        return false;
    }

    return true;
}


/**
 * @brief Perform a "sniff test" validation of aa TFTF header
 *
 * @param header The TFTF header to sniff
 *
 * @returns True if the TFTF header passes a sniff test, false otherwise
 */
bool sniff_tftf_header(tftf_header * header) {
    /* Verify the sentinel */
    return (memcmp(header->sentinel_value, tftf_sentinel, TFTF_SENTINEL_SIZE) == 0);
    }
