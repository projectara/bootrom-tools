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
#include "ims_test2.h"

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

    /* Sanity check */
    if (!prng_seed_file && !prng_seed_string){
        fprintf (stderr, "ERROR: Missing random seed\n");
        status = EINVAL;
        goto ims_init_err;
    } else if (!database_name){
        fprintf (stderr, "ERROR: No database file specified\n");
        status = EINVAL;
        goto ims_init_err;
    }

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
 * @brief Verify a message signed with the RSA signature
 *
 * Read the signature from a file and use it to verify the message.
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_rsa.c
 *
 * @param fname The name of the file containing signature
 * @param message The message to verify
 *
 * @param message The message to verify
 *
 * @returns Zero if successful, -1 otherwise.
 */
static int rsa_read_verify(const char * fname, mcl_octet * message) {
    int status = 0;
    static char s[MCL_RFS];
    mcl_octet S={0, sizeof(s), s};

    status = read_octets(fname, &S, 1);
    if (status == 0) {
        status = rsa_verify_message(message, &S);
        if (status == 0) {
            printf("ERRK verified OK\n");
        } else {
            printf("ERRK failed verification\n");
        }
    }
    return status;
}


/**
 * @brief Verify a message signed with the ECC primary/secondary signature
 *
 * Read the signature from a file and use it to verify the message.
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_ecdh.c
 *
 * @param fname The name of the file containing signature
 * @param message The message to verify
 * @param primary If true, use EPSK, if false, use ESSK
 *
 * @returns Zero if successful, -1 otherwise.
 */
static int ecc_read_verify(const char * fname, mcl_octet * message, bool primary) {
    int status = 0;
    int i;
    /* NOTE: CS, DS need to be sized to max(EPSK_SIZE, ESSK_SIZE) */
    static char cs[128];
    static char ds[128];
    mcl_octet CS_DS[2] = {{0, sizeof(cs), cs}, {0, sizeof(ds), ds}};

    read_octets(fname, CS_DS, 2);
    if (status == 0) {
#if MCL_CURVETYPE!=MCL_MONTGOMERY
        status = ecc_verify_message(message, &CS_DS[0], &CS_DS[1], primary);
        if (status == 0) {
            printf("%s verified OK\n", primary? "EPSK/EPVK" : "ESSK/ESVK");
        } else {
            printf("%s failed verification\n", primary? "EPSK/EPVK" : "ESSK/ESVK");
        }
#else
        status = -1;
#endif /* MCL_CURVETYPE!=MCL_MONTGOMERY */
    }

    return status;
}


/**
 * @brief Test verification
 *
 * Verifies a file with the different keys extracted by ims_test1.
 *
 * @param message_filename The name of the message file to verify
 * @param ep_uid_filename The name of the file containing the EP_UID
 * @param database_filename The filename of the key database
 * @param signature_filename The name of the file containing the signature
 * @param key_type The type of the signature (KEYTYPE_EPSK, KEYTYPE_ESSK or
 *        KEYTYPE_ERRK)
 * @param sample_compatibility_mode If true, generate IMS values that are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, use the correct form.
 *
 * @returns Zero if all tested IMS values verify, errno or -1 otherwise.
 */
int test_ims_verify(const char * message_filename,
                    const char * ep_uid_filename,
                    const char * database_filename,
                    const char * signature_filename,
                    const int key_type,
                    bool sample_compatibility_mode) {
    int status = 0;
    mcl_octet message={0, 0, NULL};


    /* Read in the EP_UID */
    if (status == 0) {
        status = read_octets(ep_uid_filename, &ep_uid, 1);
    }

    /* Use the EP_UID to fetch the key set from the database */
    if (status == 0) {
        status = db_get_keyset(&ep_uid, &epvk, &esvk, &erpk_mod);
        rsa_public.e = ERPK_EXPONENT;
        MCL_FF_fromOctet_C25519(rsa_public.n, &erpk_mod, MCL_FFLEN);
   }
#if 0
    /* Optionally display the ep_uid & keys for debugging */
    display_binary_data(ep_uid.val, ep_uid.len, true, "ep_uid ");
    display_binary_data(epvk.val, epvk.len, true, "epvk   ");
    display_binary_data(esvk.val, esvk.len, true, "esvk   ");
    display_binary_data(erpk_mod.val, erpk_mod.len, true, "erpk   ");
#endif

    /* Read in the message to be signed */
    if (status == 0) {
        status = read_file_into_octet(message_filename, &message);
    }

    /* Verify the message */
    if (status == 0) {
        switch (key_type) {
        case KEYTYPE_EPSK:
            status = ecc_read_verify(signature_filename, &message, true);
            break;

        case KEYTYPE_ESSK:
            status = ecc_read_verify(signature_filename, &message, false);
            break;

        case KEYTYPE_ERRK:
            status = rsa_read_verify(signature_filename, &message);
            break;

        default:
            fprintf(stderr, "ERROR: Unknown signature type\n");
            status = EINVAL;
            break;
        }
    }

    /* Cleanup */
    if (message.val) {
        free(message.val);
        message.val = NULL;
        message.len = message.max = 0;
    }

    return status;
}
