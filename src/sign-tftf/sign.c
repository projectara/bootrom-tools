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
 * @brief: This file contains the signing code for "sign-tftf" a Linux
 * command-line app used to sign one or more Trusted Firmware Transfer
 * Format (TFTF) files used by the secure bootloader.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
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
#include "util.h"
#include "parse_support.h"
#include "crypto.h"
#include "tftf.h"
#include "tftf_common.h"
#include "tftf_in.h"
#include "tftf_out.h"
#include "tftf_map.h"
#include "tftf_print.h"
#include "sign.h"


/**
 * @brief Returns the name of the signature key type
 *
 * @param package_type The signature key type
 *
 * @returns Returns a valid string on success, an error string on failure.
 *          Note that, where we are used, signature_algorithm has already
 *          been vetted.
 */
const char * get_key_name(const uint32_t package_type) {
    const char * name;

    name = token_to_kw(package_type, package_types);
    if (!name) {
        name = "ERROR";
    }

    return name;
}


/**
 * @brief Returns the algorithm name
 *
 * @param signature_algorithm The signature algorithm used
 *
 * @returns Returns a valid string on success, an error string on failure.
 *          Note that, where we are used, signature_algorithm has already
 *          been vetted.
 */
const char * get_signature_algorithm_name(const uint32_t signature_algorithm) {
    const char * name;

    name = token_to_kw(signature_algorithm, signature_algorithms);
    if (!name) {
        name = "ERROR";
    }

    return name;
}


/**
 * @brief Derive the name of the key from the key's filename
 *
 * @param signature_format The pathname to the TFTF file to sign.
 * @param package_type The pathname to the TFTF file to sign.
 * @param signature_algorithm The pathname to the TFTF file to sign.
 * @param key_filename The pathname to the TFTF file to sign.
 * @param suffix The optional suffix to append to the name
 *        ("keys.projectara.com" is used if omitted.)
 *
 * @returns True on success, false on failure
 */
char * format_key_name(const uint32_t signature_format,
                       const uint32_t package_type,
                       const uint32_t signature_algorithm,
                       const char * key_filename,
                       const char * suffix) {
    static char key_name_buf[256];
    char * key_name = key_name_buf;
    char * loc_key_filename = NULL;

    key_name_buf[0] = '\0';

    /* Create a local copy of the key_filename */
    loc_key_filename = malloc(strlen(key_filename) + 1);
    if (!loc_key_filename) {
        fprintf(stderr,
                "ERROR (format_key_name): can't alloc. local key_filename\n");
        return key_name_buf;
    }
    strcpy(loc_key_filename, key_filename);


    /* Use a default suffix if none provided */
    if (!suffix) {
        suffix = "keys.projectara.com";
    }

    /**
     * Strip any ".pem", ".private.pem" or ".public.pem" extensions from
     * the filename
     */
    rchop(loc_key_filename, ".private.pem");
    rchop(loc_key_filename, ".public.pem");
    rchop(loc_key_filename, ".pem");

    /* Generate the key name based on the format */
    switch (signature_format) {
    case FORMAT_TYPE_STANDARD:
        /**
         * In *standard* format, the name for each key is constructed by using
         * the key filename, with the *.public.pem* or *.pem* suffix removed
         * as the left half of the name, followed by an *@* symbol, followed
         * the right half of the name comprising the _Type_, a period, and the
         * _Suffix_.
         */
        snprintf(key_name_buf, sizeof(key_name_buf), "%s@%s.%s",
                 loc_key_filename,
                 get_key_name(package_type),
                 suffix);
        break;

    case FORMAT_TYPE_ES3:
        /**
         * In *es3* format, the name for each key is constructed by using the
         * key filename, with the *.public.pem* or *.pem* suffix removed as the
         * left half of the name, followed by an *@* symbol, followed the right
         * half of the name comprising the _Algorithm_, a period, and the
         * _Suffix_. The key _Type_ is not included in "es3"-format key name.
         */
        snprintf(key_name_buf, sizeof(key_name_buf), "%s@%s.%s",
                 loc_key_filename,
                 get_signature_algorithm_name(signature_algorithm),
                 suffix);
        break;

    default:
        fprintf(stderr, "ERROR: Unknown key format %d\n", signature_format);
        key_name = NULL;
    }

    if (loc_key_filename) {
        free(loc_key_filename);
    }

    return key_name;
}


/**
 * @brief Sign a TFTF
 *
 * @param filename The pathname to the TFTF file to sign.
 * @param signature_format The pathname to the TFTF file to sign.
 * @param package_type The pathname to the TFTF file to sign.
 * @param signature_algorithm The pathname to the TFTF file to sign.
 * @param key_filename The pathname to the TFTF file to sign.
 * @param suffix The optional suffix to append to the name
 *
 * @returns True on success, false on failure
 */
bool sign_tftf(const char * filename,
               const int32_t signature_format,
               const uint32_t package_type,
               const uint32_t signature_algorithm,
               const char * key_filename,
               const char * suffix) {
    bool success = false;
    size_t tftf_size;
    tftf_header * tftf_hdr = NULL;
    char * loc_key_filename = NULL;

    printf("+sign_tftf:\n"); /*****/
    /* Sanity check */
    if (!filename || !key_filename) {
        fprintf (stderr, "ERROR (sign_tftf): invalid parameters\n");
        return false;
    }


    /* Create a local copy of the key_filename */
    loc_key_filename = malloc(strlen(key_filename) + 1);
    if (!loc_key_filename) {
        fprintf(stderr,
                "ERROR (sign_tftf): can't alloc. local key_filename\n");
        return false;
    }
    strcpy(loc_key_filename, key_filename);


    /* Read in the TFTF file as a blob */
    tftf_hdr = (tftf_header *)alloc_load_file(filename, &tftf_size);
    if (tftf_hdr) {
        uint8_t *       signable_start = NULL;
        size_t          signable_length = 0;
        tftf_signature  signature_block;

        /* Initialize the signature block */
        signature_block.length = sizeof(signature_block);
        signature_block.type = signature_algorithm;
        strcpy_s (signature_block.key_name,
                  sizeof(signature_block.key_name),
                  format_key_name(signature_format,
                                  package_type,
                                  signature_algorithm,
                                  basename(loc_key_filename),
                                  suffix));

        /* Extract the signable blob from the TFTF and sign it */
        success = tftf_get_signable_region(tftf_hdr,
                                           &signable_start,
                                           &signable_length);
#if 0
        /* Calculate the signature and store it in the signature block */
        MsgDigest = M2Crypto.EVP.MessageDigest(hash_algorithm)
        MsgDigest.update(signable_blob)
        signature = key.sign(MsgDigest.digest(), hash_algorithm)
//      signature_block.signature[256];
#endif

        /* Append the signature block to the TFTF */
        success = tftf_add_section(&tftf_hdr, TFTF_SECTION_SIGNATURE, 0, 0,
                                   DATA_ADDRESS_TO_BE_IGNORED,
                                   (uint8_t*)&signature_block,
                                   sizeof(signature_block));

        /* Rewrite the file with the amended TFTF */
        if (success) {
            write_tftf_file(tftf_hdr, filename);
        }
    }

    if (loc_key_filename) {
        free(loc_key_filename);
    }
    printf("-sign_tftf: %d\n", success); /*****/
    return success;
}
