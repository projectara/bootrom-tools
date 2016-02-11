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
#include "ims_test.h"

/* Uncomment the following define to enable IMS diagnostic messages */
/*#define IMS_DEBUGMSG*/


/**
 * @brief Initialize the IMS generation subsystem
 *
 * Opens the database and IMS output file, seeds the PRNG, etc.
 *
 * @param prng_seed_file Filename from which to read the seed
 * @param prng_seed_string Raw seed string
 * @param database_name The name of the key database
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

    if (!prng_seed_file && !prng_seed_string){
        fprintf (stderr, "ERROR: Missing random seed\n");
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
    status = db_init(database_name);
    if (status != 0) {
        goto ims_init_err;
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
 * @brief Test that the RSA-2048 sign/verify works
 *
 * Sign a string with the public key and verify it with the private key.
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_rsa.c
 *
 * @param private_key A pointer to the RSA private key
 * @param public_key A pointer to the RSA public key
 *
 * @returns Zero if successful, -1 otherwise.
 */
static int test_rsa_sign_roundtrip(void) {
    int status = 0;
    char * test_string = "Hello world";
    static char m[MCL_RFS];
    static char c[MCL_RFS];
    static char s[MCL_RFS];
    static char ml[MCL_RFS];
    mcl_octet M={0, sizeof(m), m};
    mcl_octet S={0,sizeof(s),s};
    mcl_octet C={0, sizeof(c), c};
    mcl_octet ML={0, sizeof(ml), ml};

    /* Create the message M */
    MCL_OCT_jstring(&M, test_string);
#if 0
    /* PKCS V1.5 padding of a message prior to RSA signature */
    if (MCL_PKCS15_RSA2048(MCL_HASH_TYPE_RSA, &M, &C) != 1) {
        fprintf(stderr, "Unable to pad message prior to RSA signature\n");
        status = -1;
    } else {
        /* create signature in S */
        MCL_RSA_DECRYPT_RSA2048(&rsa_private, &C, &S);

        /* Verify the signature */
        MCL_RSA_ENCRYPT_RSA2048(&rsa_public, &S, &ML);
        if (MCL_OCT_comp(&C,&ML)) {
          status = 0;
        } else {
          printf("Signature is INVALID\r\n");
          printf("Signature= ");
          MCL_OCT_output(&S);
          printf("\r\n");
          status = -1;
        }
    }
#else
    status = rsa_sign_message(&M, &S);
    if (status == 0) {
        status = rsa_verify_message(&M, &S);
    }
#endif

    return status;
}


/**
 * @brief Test that the RSA-2048 encrypt-decrypt works
 *
 * Encrypt a string with the public key and decrypt it with the private key.
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_rsa.c
 *
 * @param private_key A pointer to the RSA private key
 * @param public_key A pointer to the RSA public key
 *
 * @returns Zero if successful, -1 otherwise.
 */
static int test_rsa_encryption_roundtrip(void) {
    int status = 0;
    int test_len;
    char * test_string = "Hello world";
    static char m[MCL_RFS];
    static char e[MCL_RFS];
    static char c[MCL_RFS];
    static char ml[MCL_RFS];
    mcl_octet M={0, sizeof(m), m};
    mcl_octet E={0, sizeof(e), e};
    mcl_octet C={0, sizeof(c), c};
    mcl_octet ML={0, sizeof(ml), ml};

    MCL_OCT_jstring(&M, test_string);
    /* OAEP encode message m -> e  */
    MCL_OAEP_ENCODE_RSA2048(MCL_HASH_TYPE_RSA, &M, rng, NULL, &E);


    /* encrypt encoded message e -> c */
    MCL_RSA_ENCRYPT_RSA2048(&rsa_public, &E, &C);

    /* decrypt encrypted message c -> ml */
    MCL_RSA_DECRYPT_RSA2048(&rsa_private, &C, &ML);

    /* decode decrypted message ml -> ml */
    MCL_OAEP_DECODE_RSA2048(MCL_HASH_TYPE_RSA, NULL, &ML);

    /* Verify that the decrypt matches the plaintext */
    if (memcmp(test_string, ML.val, ML.len) != 0) {
        fprintf(stderr, "ERROR: RSA encrypt/decrypt failed\n");
        fprintf(stderr, "decoded string test string '%s' (%u long)\n",
                test_string, (uint32_t)strlen(test_string));
        fprintf(stderr, "                        => '%s' (%u long)\n",
                ML.val, (uint32_t)strlen(ML.val));
        status = -1;
    }

    return status;
}


/**
 * @brief Test that the primary/secondary ECC key sign-verify works
 *
 * Sign a message with the private signing key and verify it with the public
 * verification key.
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_ecdh.c
 *
 * @param pub A pointer to the public key
 * @param priv A pointer to the private key
 * @param test_string The string to encode/decode
 *
 * @returns Zero if successful, -1 otherwise.
 */
static int test_ecc_sign_roundtrip(bool primary) {
    int status = 0;
    int i;
    /* NOTE: CS, DS need to be sized to max(EPSK_SIZE, ESSK_SIZE) */
    static char m[32];
    static char cs[128];
    static char ds[128];
    mcl_octet M={0,sizeof(m),m};
    mcl_octet CS={0,sizeof(cs),cs};
    mcl_octet DS={0,sizeof(ds),ds};

#if MCL_CURVETYPE!=MCL_MONTGOMERY

    /* Generate the message to sign */
    M.len = 17;
    for (i=0; i<=16 ;i++) {
        M.val[i] = i;
    }

<<<<<<< HEAD
    status = ecc_sign_message(&M, &CS, &DS, primary);
    if (status == 0) {
        status = ecc_verify_message(&M, &CS, &DS, primary);
=======
    /**
     * Sign the message M with private signing key epsk, outputting two
     * signature components: CS & DS
     */
    if (primary) {
        mcl_status = MCL_ECPSP_DSA_C448(MCL_HASH_TYPE_ECC, &rng, &epsk, &M,
                                        &CS, &DS);
    } else {
        mcl_status = MCL_ECPSP_DSA_C25519(MCL_HASH_TYPE_ECC, &rng, &essk, &M,
                                          &CS, &DS);

    }
    if (mcl_status !=0 ) {
        printf("ERROR: %s ECDSA Signature Failed\r\n", key_name);
    }

    /**
     * Verify the signature: verify message M with public verification key
     * epvk and the two components, CS & DS generated from signing M above.
     */
    if (primary) {
        mcl_status = MCL_ECPVP_DSA_C448(MCL_HASH_TYPE_ECC, &epvk, &M,
                                        &CS, &DS);
    } else {
        mcl_status = MCL_ECPVP_DSA_C25519(MCL_HASH_TYPE_ECC, &esvk, &M,
                                          &CS, &DS);
    }
    if (mcl_status != 0) {
      fprintf(stderr, "ERROR: %s ECC failed:\r\n",
              key_name);

      printf("Message M = 0x");
      MCL_OCT_output(&M);
      printf("\r\n");
      printf("Signature C = 0x");
      MCL_OCT_output(&CS);
      printf("\r\n");
      printf("Signature D = 0x");
      MCL_OCT_output(&DS);
      printf("\r\n");
      status = -1;
    } else {
      status = 0;
>>>>>>> Bootrom Tools: Fix "make install" error
    }
#else
    status = -1;
#endif /* MCL_CURVETYPE!=MCL_MONTGOMERY */

    return status;
}


/**
 * @brief Test an IMS value
 *
 * Test an IMS value...
 *
 * @param ims A pointer to the IMS
 * @param ims_sample_compatibility If true, extracted keys are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, extracted keys use
 *        the correct form.
 *
 * @returns Zero if successful, errno otherwise.
 */
int test_ims(uint8_t * ims, bool ims_sample_compatibility) {
    int status = 0;
    static uint8_t  epvk_buf_db[EPVK_SIZE];
    static uint8_t  esvk_buf_db[ESVK_SIZE];
    static uint8_t  erpk_mod_buf_db[ERRK_PQ_SIZE*2];
    mcl_octet epvk_db = {0, sizeof(epvk_buf_db), epvk_buf_db};
    mcl_octet esvk_db = {0, sizeof(esvk_buf_db), esvk_buf_db};
    mcl_octet erpk_mod_db = {0, sizeof(erpk_mod_buf_db), erpk_mod_buf_db};

    /**
     * Calculate the Endpoint Unique ID (EP_UID) from the IMS and the
     * private/public keys.
     */
    calculate_keys(ims, ims_sample_compatibility);

    /* Compare the calculated public keys with those from the database */
    status = db_get_keyset(&ep_uid, &epvk_db, &esvk_db, &erpk_mod_db);
    if (status != SQLITE_OK) {
        fprintf(stderr, "ERROR: Can't find EP_UID in db\n");
        display_binary_data(ep_uid.val, ep_uid.len, true, "epu_id ");
        status = -1;
    } else {
        if (!MCL_OCT_comp(&epvk, &epvk_db)) {
            fprintf(stderr, "ERROR: extracted EPVK doesn't match db:\n");
            display_binary_data(ep_uid.val, ep_uid.len, true, "epu_id ");
            display_binary_data(epvk.val, epvk.len, true, "epvk     ");
            status = -1;
        }
        if (!MCL_OCT_comp(&esvk, &esvk_db)) {
            fprintf(stderr, "ERROR: extracted ESVK doesn't match db:\n");
            display_binary_data(ep_uid.val, ep_uid.len, true, "epu_id ");
            display_binary_data(esvk.val, esvk.len, true, "esvk     ");
            status = -1;
        }
        if (!MCL_OCT_comp(&erpk_mod, &erpk_mod_db)) {
            fprintf(stderr, "ERROR: extracted ERPK_MOD doesn't match db\n");
            display_binary_data(ep_uid.val, ep_uid.len, true, "epu_id ");
            display_binary_data(erpk_mod.val, erpk_mod.len, true, "erpk_mod ");
            status = -1;
        }
    }

    /**
     * Verify RSA and primary and secondary ECC signing work
     */
    if (status == 0) {
        status = test_rsa_sign_roundtrip();
    }
    if (status == 0) {
        status = test_ecc_sign_roundtrip(true);
    }
    if (status == 0) {
        status = test_ecc_sign_roundtrip(false);
    }

    return status;
}


/**
 * @brief Read and verify an IMS value
 *
 * @param ims_fd The file descriptor of the IMS file
 * @param index Which IMS value to verify (zero-based)
 * @param sample_compatibility_mode If true, generate IMS values that are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, generate the IMS value using
 *        the correct form.
 *
 * @returns Zero if the IMS value verified, errno or -1 otherwise.
 */
int read_verify_ims(const int ims_fd, const uint32_t index,
                 bool sample_compatibility_mode) {
    off_t offset;
    int status;

    offset = index * IMS_LINE_SIZE;
    printf("IMS[%u]\n", index);
    status = ims_read(ims_fd, offset, ims);
    if (status == 0) {
        status = test_ims(ims, sample_compatibility_mode);
    }

    return status;
}


/**
 * @brief Test a representative sample of IMS values
 *
 * Reads a random sampling of N IMS values, extracts the key values
 * from them and verifies that sample text can be encrypted-decrypted
 * with them.
 *
 * @param ims_filename The name of the IMS input file
 * @param num_ims The number of IMS values to test
 * @param sample_compatibility_mode If true, generate IMS values that are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, generate the IMS value using
 *        the correct form.
 *
 * @returns Zero if all tested IMS values verify, errno or -1 otherwise.
 */
int test_ims_set(const char * ims_filename, uint32_t num_ims,
                 bool sample_compatibility_mode) {
    int status = 0;
    int ims_fd;
    uint32_t i;
    uint32_t j;
    uint32_t index;
    uint32_t r;
    bool unique;
    off_t offset;
    int num_available_ims;
    int * test_set = NULL;
    struct stat ims_stat = {0};

    ims_fd = open(ims_filename, O_RDONLY);
    if (ims_fd  == -1) {
        fprintf(stderr, "ERROR: Can't open IMS file '%s'\n", ims_filename);
        status = errno;
    } else {
        /* Determine how many IMS values are in the file. */
        num_available_ims = num_ims_in_file(ims_filename);
        if (num_available_ims > 0) {
            if (num_ims > num_available_ims) {
                fprintf(stderr, "Warning: IMS file only contains %d entr%s\n",
                        num_available_ims,
                        (num_available_ims == 1)? "y" : "ies");
                num_ims = num_available_ims;
            }

            printf("Test %d of %d IMS values%s\n", num_ims, num_available_ims,
                    sample_compatibility_mode?
                            " (compatible with initial 100 IMS samples)" :
                            "");

            if (num_ims >= num_available_ims) {
                /* Sequentially scan all IMS */
                for (i = 0; (i < num_ims) && (status == 0); i++) {
                    status = read_verify_ims(ims_fd, i,
                                             sample_compatibility_mode);
                }
            } else {
                /* Randomly draw and verify N IMS values from the IMS file */
                test_set = calloc(num_ims, sizeof(*test_set));
                if (test_set) {
                    /* Randomly draw N unique IMS values */
                    /* Create a set of unique indices */
                    for (i = 0; (i < num_ims) && (status == 0); i++) {
                        do {
                            unique = true;
                            /* Draw a random index and check for uniqueness */
                            r = rand32() % num_available_ims;
                            for (j = 0; j < i; j++) {
                                if (r == test_set[j]) {
                                    /* Duplicate, draw again */
                                    unique = false;
                                    break;
                                }
                            }
                        } while (!unique);
                        test_set[i] = r;
                    }
                    /* Process the set of unique indices */
                    for (i = 0; (i < num_ims) && (status == 0); i++) {
                        status = read_verify_ims(ims_fd, test_set[i],
                                                 sample_compatibility_mode);
                    }
                    free(test_set);
                } else {
                    /* Draw N IMS values, with some risk of duplication */
                    fprintf(stderr,
                            "Warning: random sampling may have duplicates\n");
                    for (i = 0; (i < num_ims) && (status == 0); i++) {
                        /* Randomly draw IMS */
                        status = read_verify_ims(ims_fd,
                                                 rand32() % num_available_ims,
                                                 sample_compatibility_mode);
                     }
                }
            }
        }

        close (ims_fd);
    }

    return status;
}
