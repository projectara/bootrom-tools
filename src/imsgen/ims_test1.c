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
#include "ims_test_core.h"
#include "ims_test1.h"

/* Uncomment the following define to enable IMS diagnostic messages */
/*#define IMS_DEBUGMSG*/


/**
 * @brief Initialize the IMS generation subsystem
 *
 * Opens the database and IMS output file, seeds the PRNG, etc.
 *
 * @param prng_seed_file Filename from which to read the seed
 * @param prng_seed_string Raw seed string
 * @param database_name (optional) The name of the key database
 *
 * @note One and only 1 of prng_seed_file and prng_seed_string must be
 *       non-null.
 *
 * @returns Zero if successful, errno otherwise.
 */
int ims_init(const char * prng_seed_file,
             const char * prng_seed_string,
             const char * database_name) {
    int status = 0;
    mcl_octet * seed = NULL;

    /* Seed the PRNG */
    status = ims_common_init(prng_seed_file, prng_seed_string);
    if (status != 0) {
        goto ims_init_err;
    }
    MCL_RAND_seed(&rng, prng_seed_length, prng_seed_buffer);

    /* Open the key database */
    if (database_name) {
        status = db_init(database_name);
        if (status != 0) {
            goto ims_init_err;
        }
    }

ims_init_err:

    return status;
}


/**
 * @brief De-initialize the IMS generation subsystem
 *
 * Flushes the IMS output file, closes the database
 */
void ims_deinit(void) {
    /* Close the key database */
    db_deinit();

    ims_common_deinit();
}


/**
 * @brief Test that the RSA sign works
 *
 * Sign a message with the RSA signing key, and write the signature to a
 * file.
 *
 * @param message The message to sign
 *
 * @returns Zero if successful, -1 otherwise.
 */
static int rsa_sign_save(mcl_octet * message) {
    int status = 0;
    static char c[MCL_RFS];
    static char s[MCL_RFS];
    mcl_octet S={0,sizeof(s),s};
    mcl_octet C={0, sizeof(c), c};

    status = rsa_sign_message(message, &S);
    if (status == 0) {
        /* write the signature to a file */
        status = write_octets(FNAME_RSA_SIG, &S, 1);
    }
    return status;
}


/**
 * @brief Test that the primary/secondary key ECC sign works
 *
 * Sign a message with the ECC signing key, and write the signatures to a
 * file.
 *
 * @param message The message to sign
 * @param primary If true, use EPSK, if false, use ESSK
 *
 * @returns Zero if successful, -1 otherwise.
 */
static int ecc_sign_save(mcl_octet * message, bool primary) {
    int status = 0;
    int i;
    /* NOTE: CS, DS need to be sized to max(EPSK_SIZE, ESSK_SIZE) */
    static char cs[128];
    static char ds[128];
    mcl_octet CS_DS[2] = {{0, sizeof(cs),cs}, {0, sizeof(ds),ds}};

#if MCL_CURVETYPE!=MCL_MONTGOMERY
    status = ecc_sign_message(message, &CS_DS[0], &CS_DS[1], primary);
    if (status == 0) {
        /* Write the CS & CDS signature buffers to a file */
        status = write_octets(primary? FNAME_ECC_PRIMARY_SIG :
                                       FNAME_ECC_SECONDARY_SIG,
                              CS_DS, 2);
    }
#else
    status = -1;
#endif /* MCL_CURVETYPE!=MCL_MONTGOMERY */

    return status;
}


/**
 * @brief Test signing
 *
 * Signs a file with the different keys extracted from an IMS.
 *
 * Note that if ims_binascii is non-null, ims_filename and ims_index are
 * ignored. If ims_value is null, we use ims_filename and ims_index.
 *
 * @param message_filename The name of the message file to sign
 * @param ims_value (optional) The binascii string representation of the IMS
 * @param ims_filename (optional) The name of the IMS input file
 * @param ims_index (optional) Which IMS value to use from ims_filename
 * @param sample_compatibility_mode If true, generate IMS values that are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, generate the IMS value using
 *        the correct form.
 *
 * @returns Zero if all tested IMS values verify, errno or -1 otherwise.
 */
int test_ims_signing(const char * message_filename,
                     const char * ims_binascii,
                     const char * ims_filename,
                     const int    ims_index,
                     bool sample_compatibility_mode) {

    int status = 0;
    mcl_octet message={0, 0, NULL};

    /* Obtain the IMS value */
    status = get_ims(ims_binascii, ims_filename, ims_index, ims);

    /* Read in the message to be signed */
    if (status == 0) {
        status = read_file_into_octet(message_filename, &message);
    }

    /* Extract our signing keys from the IMS and use them to sign the message */
    if (status == 0) {
        /* RSA signing */
        calculate_keys(ims, sample_compatibility_mode);
        status = write_octets(FNAME_EP_UID, &ep_uid, 1);

        display_binary_data(ep_uid.val, ep_uid.len, true, "ep_uid ");
#if 0
        /* optionally print the keys for debugging */
        display_binary_data(epvk.val, epvk.len, true, "epvk   ");
        display_binary_data(esvk.val, esvk.len, true, "esvk   ");
        display_binary_data(erpk_mod.val, erpk_mod.len, true, "erpk   ");
#endif

    }
    if (status == 0) {
        status = rsa_sign_save(&message);
    }
    if (status == 0) {
        /* Primary ECC signing */
        status = ecc_sign_save(&message, true);
    }
    if (status == 0) {
        /* Secondary ECC signing */
        status = ecc_sign_save(&message, false);
   }

    /* Cleanup */
    if (message.val) {
        free(message.val);
        message.val = NULL;
        message.len = message.max = 0;
    }

    return status;
}
