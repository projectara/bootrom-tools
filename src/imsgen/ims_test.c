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
#include "ims_test.h"

/**
 * Because IMS values are stored as binary ascii text with trailing newlines,
 * define contants for how much to read and how far to seek
 */
#define IMS_BINASCII_SIZE    (IMS_SIZE * 8)
#define IMS_LINE_SIZE   (IMS_BINASCII_SIZE + 1)


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
    /*****/printf("ims_test::ims_init\n");
    int status = 0;
    mcl_octet * seed = NULL;

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
#if 0
    {
        uint32_t test_ims[9]={
                0xb9607267,
                0x60a3f8aa,
                0xcda468be,
                0x7c67f4a2,
                0x362c1a89,
                0x018fc2fe,
                0x39223191,
                0xf66da6db,
                  0x470daa};
        uint32_t test_ims1[9]={
                0xc7413024,
                0x1fc13463,
                0x42efbeef,
                0x7c67f4a2,
                0x362c1a89,
                0x018fc2fe,
                0x39223191,
                0xf66da6db,
                  0x470daa};
        uint32_t test_ims2[9] = {
                0x1a5ccadb,
                0xda83f60b,
                0xc7139162,
                0xb80e09ee,
                0x4a53f6a4,
                0x8abfb832,
                0xbc7a7ccd,
                0x0848c95e,
                  0xd2c7f0};
        char test_ims_digest[SHA256_HASH_DIGEST_SIZE];
        hash_it((uint8_t*)test_ims2, IMS_SIZE, test_ims_digest);
        display_binary_data(test_ims_digest, sizeof(test_ims_digest), true, "ims hash = ");
    }
#endif

ims_init_err:

    return status;
}


/**
 * @brief De-initialize the IMS generation subsystem
 *
 * Flushes the IMS output file, closes the database
 */
void ims_deinit(void) {
    /*****/printf("ims_test::ims_deinit\n");
    /* Close the key database */
    db_deinit();

    ims_common_deinit();
}


/**
 * @brief Read the IMS as an ASCII binarray string
 *
 * Reads the IMS value as a signle line binary array string, MSB-to-LSB
 *
 * @param fd The file to read
 * @param offset Where in the file from which to read
 * @param ims The IMS value to load
 *
 * @returns Zero if successful, errno otherwise.
 */
int ims_read(int fd, off_t offset, uint8_t * ims) {
    int status = 0;
    char binascii_buf[IMS_BINASCII_SIZE];
    int  i = 0;
    int byte_index;
    int bit;
    uint8_t byte;

    /* Seek to the desired entry and read the binascii */
    if (lseek(fd, offset, SEEK_SET) < 0) {
        fprintf(stderr, "ERROR: Can't seek to %u in IMS file (err %d)\n",
                (uint32_t)offset, errno);
        status = errno;
    } else {
        if (read(fd, binascii_buf, sizeof(binascii_buf)) <= 0) {
            fprintf(stderr, "ERROR: Can't read IMS file (err %d)\n", errno);
            status = errno;
        } else {
            /* Assemble the IMS byte-by-byte */
            for (byte_index = IMS_SIZE - 1; byte_index >= 0; byte_index--) {
                /* Assemble a byte */
                byte = 0;
                for (bit = 0; bit < 8; bit++) {
                    byte <<= 1;
                    if (binascii_buf[i] == '1') {
                        byte |= 1;
                    } else if (binascii_buf[i] != '0') {
                        fprintf(stderr, "ERROR: IMS file contains garbage (%c)\n",
                                binascii_buf[i]);
                        return EIO;
                    }
                    i++;
                }
                ims[byte_index] =  byte;
            }
        }
    }
    /*****/printf("ims_test::ims_read\n");
    /*****/display_binary_data(ims, IMS_SIZE, true, NULL);
    return status;
}


/**
 * @brief Calculate the Endpoint Rsa pRivate Key (ERRK)
 *
 * @param y2 A pointer to the Y2 term used by all
 * @param ims A pointer to the ims (the upper 3 bytes will be modified)
 * @param erpk_mod A pointer to a buffer to store the modulus for ERPK
 * @param errk_d A pointer to a buffer to store the  ERPK decryption exponent
 *
 * @returns Zero if successful, errno otherwise.
 */
static int calc_errk(uint8_t * y2,
                     uint8_t * ims,
                     mcl_octet * erpk_mod,
                     mcl_octet * errk_d) {
    int status = 0;
    uint32_t p_bias;
    uint32_t q_bias;
    uint32_t pq_bias;

    /**
     *  Y2 = sha256(IMS[0:31] xor copy(0x5a, 32))  // (provided)
     *  Z3 = sha256(Y2 || copy(0x03, 32))
     *  ERRK_P[0:31] = sha256(Z3 || copy(0x01, 32))
     *  ERRK_P[32:63] = sha256(Z3 || copy(0x02, 32))
     *  ERRK_P[64:95] = sha256(Z3 || copy(0x03, 32))
     *  ERRK_P[96:127] = sha256(Z3 || copy(0x41, 32))
     *  ERRK_Q[0:31] = sha256(Z3 || copy(0x05, 32))
     *  ERRK_Q[32:63] = sha256(Z3 || copy(0x06, 32))
     *  ERRK_Q[64:95] = sha256(Z3 || copy(0x07, 32))
     *  ERRK_Q[96:127] = sha256(Z3 || copy(0x8, 32))
     *  ERRK_P[0] |= 0x01                          // force P, Q to be odd
     *  ERRK_Q[0] |= 0x01
     *  <ensure ERRK_P & ERRK_Q are within 8192 of being prime>
     *  ERPK_MOD = ERRK_Q * ERRK_P                 // generate modulus for ERPK
     *  ERPK_E = 65537                             // public exponent
     *  ERRK_D = RSA_secret(ERRK_P, ERK_Q, ERPK_E) // Private decrypt exponent
     *                                             // (unused in IMS creation)
     */
    calc_errk_pq_bias_odd(y2, ims, &errk_p, &errk_q);

    /* Convert P & Q to FF format */
    MCL_FF_fromOctet_C25519(p_ff, &errk_p, MCL_HFLEN);
    MCL_FF_fromOctet_C25519(q_ff, &errk_q, MCL_HFLEN);

#if 1
    /* Force ERRK_P, ERRK_Q to be odd 3 mod 4 */
    odd_ff_from_octet(p_ff, &errk_p, MCL_HFLEN);
    odd_ff_from_octet(q_ff, &errk_q, MCL_HFLEN);
#endif
    /*****/printf("Lower byte of P 0x%02x, Q 0x%02x\n", MCL_FF_lastbits_C25519(p_ff, 8), MCL_FF_lastbits_C25519(q_ff, 8));
    /*****/printf("Lower bits of P 0x%x, Q 0x%x\n", MCL_FF_lastbits_C25519(p_ff, 2), MCL_FF_lastbits_C25519(q_ff, 2));



    /* Extract the P & Q bias from IMS[32:34] */
    pq_bias = ims[32] | (ims[33] << 8) | (ims[34] << 16);
    p_bias = ((pq_bias / 4096) * 2);
    q_bias = ((pq_bias % 4096) * 2);
    /*****/printf("pq_bias %x, p_bias %x, q_bias %x\n", pq_bias, p_bias, q_bias);

    /* Bias P & Q */
    MCL_FF_inc_C25519(p_ff, p_bias, MCL_HFLEN);
    MCL_FF_inc_C25519(q_ff, q_bias, MCL_HFLEN);
    /*****/printf("add (p,q) += (pbias,qbias)\n");

    /**
     * Generate the private exponent.
     * On return, the structs are set to the following:
     *   - pub_key.n    ERRPK_MOD (p * q)
     *   - pub_key.e    ERPK_EXPONENT (65537)
     *   - priv_key.p   secret prime p
     *   - priv_key.q   secret prime q
     *   - priv_key.dp  decrypting exponent mod (p-1)
     *   - priv_key.dq  decrypting exponent mod (q-1)
     *   - priv_key.c   1/p mod q
     */
    MCL_FF_copy_C25519(rsa_private.p, p_ff, MCL_HFLEN);
    MCL_FF_copy_C25519(rsa_private.q, q_ff, MCL_HFLEN);
    /*****/printf("copied PQ to rsaprivate. rsa_secret...\n");
    rsa_secret(&rsa_private, &rsa_public, ERPK_EXPONENT);
    /*****/printf("rsa_secret returned...\n");

    /* Convert the calculated FF nums back into octets for later use */
    MCL_FF_toOctet_C25519(erpk_mod, rsa_public.n, MCL_HFLEN);
    /*****/display_binary_data(erpk_mod->val, erpk_mod->len, true, "erpk_mod ");

    /*****/printf("calc_errk- %d\n", status);
    return status;
}


/**
 * @brief Test that the RSA-2048 encrypt-decrypt works
 *
 * Encrypt a string with the public key and decrypt it with the private key.
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_rsa.c
 *
 * @returns Zero if successful, -1 otherwise.
 */
int test_rsa_roundtrip(void) {
    int status = 0;
    int test_len;
    csprng RNG;
    char * test_string = "Hello world";
    static char m[MCL_RFS];
    static char e[MCL_RFS];
    static char c[MCL_RFS];
    static char ml[MCL_RFS];
    static mcl_octet M={0, sizeof(m), m};
    static mcl_octet E={0, sizeof(e), e};
    static mcl_octet C={0, sizeof(c), c};
    static mcl_octet ML={0, sizeof(ml), ml};

    //printf("RSA Encrypt test string '%s' (%u long)\n", test_string, (uint32_t)strlen(test_string));
    MCL_OCT_jstring(&M, test_string);
    //*****/printf("Plaintext= ");MCL_OCT_output(&M);printf("\r\n");
    /* OAEP encode message m to e  */
    MCL_OAEP_ENCODE_RSA2048(MCL_HASH_TYPE_RSA, &M, &rng, NULL, &E);
    //*****/printf("RSA Encodetext= "); MCL_OCT_output(&E); printf("\r\n");


    /* encrypt encoded message e to c */
    MCL_RSA_ENCRYPT_RSA2048(&rsa_public, &E, &C);
    //printf("RSA Ciphertext= "); MCL_OCT_output(&C); printf("\r\n");

    /* decrypt encrypted message c to ml */
    printf("Decrypting test string\n");
    MCL_RSA_DECRYPT_RSA2048(&rsa_private, &C, &ML);
    //*****/printf("RSA Decyphertext= "); MCL_OCT_output(&ML); printf("\r\n");

    /* decode decrypted message ml to ml */
    MCL_OAEP_DECODE_RSA2048(MCL_HASH_TYPE_RSA, NULL, &ML);
    //*****/printf("RSA Decodetext (%u) = ", (uint32_t)ML.len);
    //MCL_OCT_output_string(&ML); printf("\n");

    /* We see a double decrypt (e.g. "Hello world" =>
     * "Hello worldHello world"), so if the initial memcmp fails, check
     * if the decrypt is an even # of bytes and the front and back halves
     * match.
     */
    test_len = ML.len / 2;
    if (((ML.len & 0x01) == 0) &&
        (memcmp(&ML.val[0], &ML.val[test_len], test_len) == 0)) {
            fprintf(stderr, "Warning: RSA double decrypt\n");
    } else {
        /* Not a double-decrypt, use the full length in the match test below */
        test_len = ML.len;
    }

    /* Verify that the decrypt matches the plaintext */
    if (memcmp(test_string, ML.val, test_len) != 0) {
        fprintf(stderr, "ERROR: RSA encrypt/decrypt failed\n");
        fprintf(stderr, "decoded string test string '%s' (%u long)\n",
                test_string, (uint32_t)strlen(test_string));
        fprintf(stderr, "                        => '%s' (%u long)\n",
                ML.val, (uint32_t)strlen(ML.val));
        status = -1;
    } else {
        fprintf(stderr, "RSA encrypt/decrypt OK\n");
    }

    return status;
}


/**
 * @brief Test that the primary/secondary key ECC sign-verify works
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
int test_ecc_roundtrip(bool primary) {
    int status = -1;
    int mcl_status;
    int i;
    char * key_name = primary? "primary" : "secondary";
    static char s0[MCL_EGS];
    static char w0[2*MCL_EFS+1];
    static char m[32];
    static char cs[MCL_EGS];
    static char ds[MCL_EGS];
    mcl_octet S0={0,sizeof(s0),s0};
    mcl_octet W0={0,sizeof(w0),w0};
    static mcl_octet M={0,sizeof(m),m};
    mcl_octet CS={0,sizeof(cs),cs};
    mcl_octet DS={0,sizeof(ds),ds};

#if MCL_CURVETYPE!=MCL_MONTGOMERY
    /*****/printf("Testing ECDSA\r\n");
    /*****/printf("MCL_EGS %d, epsk %d\r\n", MCL_EGS, epsk.len);

    /* Generate the message to sign */
    M.len = 17;
    for (i=0; i<=16 ;i++) {
        M.val[i] = i;
    }

    /* Sign the message */
    if (primary) {
        mcl_status = MCL_ECPSP_DSA_C488(MCL_HASH_TYPE_ECC, &rng, &epsk, &M,
                                        &CS, &DS);
    } else {
        mcl_status = MCL_ECPSP_DSA_C25519(MCL_HASH_TYPE_ECC, &rng, &essk, &M,
                                          &CS, &DS);

    }
    if (mcl_status !=0 ) {
      printf("***ECDSA Signature Failed\r\n");
    }

    printf("Signature C = 0x");
    MCL_OCT_output(&CS);
    printf("\r\n");
    printf("Signature D = 0x");
    MCL_OCT_output(&DS);
    printf("\r\n");

    /* Verify the signature */
    if (primary) {
        mcl_status = MCL_ECPVP_DSA_C488(MCL_HASH_TYPE_ECC, &epvk, &M,
                                        &CS, &DS);
    } else {
        mcl_status = MCL_ECPVP_DSA_C25519(MCL_HASH_TYPE_ECC, &esvk, &M,
                                          &CS, &DS);
    }
    if (mcl_status != 0) {
      fprintf(stderr, "ERROR: ECDSA Verification Failed on %s ECC\r\n",
              key_name);
      status = -1;
    } else {
      printf("ECDSA Verification succeeded on %s ECC\r\n", key_name);
      status = 0;
    }
#endif

    return status;
}


/**
 * @brief Test an IMS value
 *
 * Test an IMS value...
 *
 * @param ims A pointer to the ims (the upper 3 bytes will be modified)
 *
 * @returns Zero if successful, errno otherwise.
 */
int test_ims(uint8_t * ims) {
    int status = 0;
    static uint8_t  epvk_buf_db[EPVK_SIZE];
    static mcl_octet epvk_db = {0, sizeof(epvk_buf_db), epvk_buf_db};
    static uint8_t  esvk_buf_db[ESVK_SIZE];
    static mcl_octet esvk_db = {0, sizeof(esvk_buf_db), esvk_buf_db};
    static uint8_t  erpk_mod_buf_db[ERRK_PQ_SIZE];
    static mcl_octet erpk_mod_db = {0, sizeof(erpk_mod_buf_db), erpk_mod_buf_db};

    /*****/printf("ims_test::test_ims+\n");
    /**
     * Calculate the Endpoint Unique ID (EP_UID) from the IMS (used to look
     * up the keys from the database).
     */
    calculate_epuid_es3(ims, &ep_uid);
    ep_uid.len = EP_UID_SIZE;
    /*****/display_binary_data(ep_uid.val, ep_uid.len, true, "test_ims epu_id ");

    /* Calculate "Y2", used in generating EPSK, MPDK, ERRK, EPCK, ERGS */
    calculate_y2(ims, y2);

    /* Calculate ERRK/ERPK, EPSK/EPVK and  ESSK/ESVK */
    calc_epsk(y2, &epsk);
    calc_epvk(&epsk, &epvk);
    calc_essk(y2, &essk);
    calc_esvk(&essk, &esvk);
    calc_errk(y2, ims, &erpk_mod, &errk_d);

    /* Compare the calculated public keys with those from the database */
    status = db_get_keyset(&ep_uid, &epvk_db, &esvk_db, &erpk_mod_db);
    if (status != SQLITE_OK) {
        fprintf(stderr, "ERROR: Can't find EP_UID in db\n");
        status = -1;
    } else {
        if (!MCL_OCT_comp(&epvk, &epvk_db)) {
            fprintf(stderr, "ERROR: extracted EPVK doesn't match db\n");
            status = -1;
        }
        if (!MCL_OCT_comp(&esvk, &esvk_db)) {
            fprintf(stderr, "ERROR: extracted ESVK doesn't match db\n");
            status = -1;
        }
        if (!MCL_OCT_comp(&erpk_mod, &erpk_mod_db)) {
            fprintf(stderr, "ERROR: extracted ERPK_MOD doesn't match db\n");
            status = -1;
        }
    }

    /**
     * Verify RSA encrypt-decrypt and primary and secondary signing work
     */
    if (status == 0) {
        /*status =*/ test_rsa_roundtrip();
        /*status =*/ test_ecc_roundtrip(true);
        /*status =*/ test_ecc_roundtrip(false);
    }


    /*****/printf("ims_test::test_ims- %d\n", status);
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
 *
 * @returns Zero if all tested IMS values verify, errno otherwise.
 */
int test_ims_set(const char * ims_filename, int num_ims) {
    int status = 0;
    int fd;
    int i;
    int index;
    off_t offset;
    int num_available_ims;
    struct stat ims_stat = {0};

    /*****/printf("ims_test::test_ims_set+\n");
    fd = open(ims_filename, O_RDONLY);
    if (open(ims_filename, O_RDONLY) == -1) {
        fprintf(stderr, "ERROR: Can't open IMS file '%s'\n", ims_filename);
        status = errno;
    } else {
        if (fstat(fd, &ims_stat) != 0) {
            fprintf(stderr, "ERROR: Can't find IMS file '%s'\n", ims_filename);
            status = errno;
        } else {
            /* Determine how many IMS values are in the file. */
            num_available_ims = ims_stat.st_size / (IMS_LINE_SIZE);
            if (num_ims > num_available_ims) {
                fprintf(stderr, "Warning: IMS file only contains %d entries\n",
                        num_available_ims);
                num_ims = num_available_ims;
            }

            /* Randomly draw <num_ims> IMS values from the IMS file and verify them */
            for (i = 0; (i < num_ims) && (status == 0); i++) {
                if (num_ims >= num_available_ims) {
                    /* Sequentially scan all IMS */
                    index = i;
                } else {
                    /* Randomly draw IMS */
                    index = random() % num_available_ims;
                }
                offset = index * IMS_LINE_SIZE;
                printf("IMS[%u]\n", index);
                status = ims_read(fd, offset, ims);
                if (status == 0) {
                    status = test_ims(ims);
                }
            }
        }

        close (fd);
    }

    /*****/printf("ims_test::test_ims_set- %d\n", status);
    return status;
}
