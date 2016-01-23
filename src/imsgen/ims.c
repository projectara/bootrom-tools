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
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/engine.h>
#include "util.h"
#include "mcl_arch.h"
#include "mcl_oct.h"
#include "mcl_ecdh.h"
#include "mcl_rand.h"
#include "mcl_rsa.h"
#include "crypto.h"
#include "db.h"
#include "ims_common.h"
#include "ims.h"

/* Uncomment the following define to enable IMS diagnostic messages */
/*#define IMS_DEBUGMSG*/

/* IMS output file */
static FILE *   fp_ims;

/**
 * Endpoint Rsa pRivate Key (ERRK/ERPK) data:
 */

/* Maximum biased ERRK P or Q before overflow */
mcl_chunk errk_max_pq_ff[MCL_HFLEN][MCL_BS];

mcl_chunk erpk_mod_ff[MCL_HFLEN][MCL_BS];
mcl_chunk erpk_e[MCL_HFLEN][MCL_BS];
mcl_chunk erpk_d[MCL_HFLEN][MCL_BS];
mcl_chunk p1[MCL_HFLEN][MCL_BS];
mcl_chunk q1[MCL_HFLEN][MCL_BS];

void calc_errk_max_pq(void);


/**
 * @brief Initialize the IMS generation subsystem
 *
 * Opens the database and IMS output file, seeds the PRNG, etc.
 *
 * @param prng_seed_file Filename from which to read the seed
 * @param prng_seed_string Raw seed string
 * @param ims_filename The name of the IMS output file
 * @param database_name The name of the key database
 *
 * @note One and only 1 of prng_seed_file and prng_seed_string must be
 *       non-null.
 *
 * @returns Zero if successful, errno otherwise.
 */
int ims_init(const char * prng_seed_file,
             const char * prng_seed_string,
             const char * ims_filename,
             const char * database_name) {
    int status = 0;
    mcl_octet * seed = NULL;

    /* Seed the PRNG */
    status = ims_common_init(prng_seed_file, prng_seed_string);

    /* Open the key database */
    status = db_init(database_name);
    if (status != 0) {
        goto ims_init_err;
    }

    /* Open the IMS output file */
    fp_ims = fopen(ims_filename, "w");

    /* Establish any really big number constants */
    calc_errk_max_pq();

ims_init_err:

    return status;
}


/**
 * @brief De-initialize the IMS generation subsystem
 *
 * Flushes the IMS output file, closes the database
 */
void ims_deinit(void) {
    /* Close the IMS output file */
    if (fp_ims) {
        fclose(fp_ims);
        fp_ims = NULL;
    }

    /* Close the key database */
    db_deinit();

    ims_common_deinit();
}


/**
 * @brief Generate an FF num for the maximum starting ERRK_P or ERRK_Q
 *
 * Because we don't want to overflow the ERRK P &Q bias calculations, we
 * must ensure that the pre-bias P and Q are no bigger than 2^1024 - (1 + 8192)
 */
void calc_errk_max_pq(void) {
    uint8_t  errk_max_pq_buf[ERRK_PQ_SIZE];
    mcl_octet errk_max_pq = {0, sizeof(errk_max_pq_buf), errk_max_pq_buf};
    int i;

    /* Define the maximal number (2^1024 - 1) as a constant octet */
    for (i = 0; i < errk_max_pq.max; i++) {
        errk_max_pq.val[i] = 0xff;
    }

    /* Convert the octet into an FF */
    MCL_FF_fromOctet_C25519(errk_max_pq_ff, &errk_max_pq, MCL_HFLEN);

    /* Decrement the FF by our threshold. Since we'll be checking in a loop
     * as we continue to increase the bias (and thus P & Q), we can get
     * greedy and stop at (2^1024 - 1) - 2 (i.e., the last increment) instead
     * of (2^1024 - 1) - 8192
     */
    MCL_FF_dec_C25519(errk_max_pq_ff, 2, MCL_HFLEN);
}


/**
 * @brief Write the IMS as an ASCII binarray string
 *
 * Writes the IMS value as a single-line binary array string, MSB-to-LSB
 *
 * @param fp The file to which to write
 * @param epsk The IMS value to write
 *
 * @returns Zero if successful, errno otherwise.
 */
int ims_write(FILE * fp, uint8_t * ims) {
    int status = 0;
    int byte_index;
    int bit;
    uint8_t byte;

#ifdef IMS_DEBUGMSG
    printf("ims_write:\n");
    display_binary_data(ims, IMS_SIZE, true, NULL);
#endif
   if (fp_ims) {
        for (byte_index = IMS_SIZE - 1; byte_index >= 0; byte_index--) {
            byte = ims[byte_index];
            for (bit = 0; bit < 8; bit++) {
                if (fputc((byte & BYTE_MASK_MSB)? '1' : '0', fp) == EOF) {
                    return EIO;
                }
                byte <<= 1;
            }
        }
        if (fputc('\n', fp) == EOF) {
                 return EIO;
        }

        return 0;
    } else {
        return EBADF;
    }
}


/**
 * @brief Generate a candidate IMS value
 *
 * Generates a random IMS value, the lower 32-bits of which will have a
 * Hamming weight of 128.
 *
 * @param ims A pointer to the 35-byte IMS value
 */
static void ims_generate_candidate(uint8_t * ims) {
    int status = 0;
    int i;

    do {
        /* Create a new 35-bit random number... */
        for(i = 0; i < IMS_HAMMING_SIZE; i++) {
            ims[i] = MCL_RAND_byte(&rng);
        }
        /* ...and check the Hamming weight of the lower 32 bytes) */
    } while (hamming_weight(ims, IMS_HAMMING_SIZE) != IMS_HAMMING_WEIGHT);
}


/**
 * @brief Calculate the Endpoint Rsa pRivate Key (ERRK)
 *
 * @param y2 A pointer to the Y2 term used by all
 * @param ims A pointer to the ims (the upper 3 bytes will be modified)
 * @param erpk_mod A pointer to a buffer to store the modulus for ERPK
 * @param errk_d A pointer to a buffer to store the  ERPK decryption exponent
 * @param ims_sample_compatibility If true, generate IMS values that are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, generate the IMS value using
 *        the correct form.
 *
 * @returns Zero if successful, errno otherwise.
 */
static int calc_errk(uint8_t * y2,
                     uint8_t * ims,
                     mcl_octet * erpk_mod,
                     mcl_octet * errk_d,
                     bool ims_sample_compatibility) {
    int status = 0;
    uint32_t p_bias;
    uint32_t q_bias;
    uint32_t pq_bias;
    MCL_rsa_private_key priv_key = { 0 };
    MCL_rsa_public_key pub_key = { 0 };
    int odd_mod;
    int prime_search_limit;
    uint8_t odd_mod_bitmask;

    /**
     * Define constants based on compatibility with the original 100 IMS samples
     * delivered to Toshiba or the correct production form.
     */
    odd_mod = (ims_sample_compatibility)? ODD_MOD_SAMPLE : ODD_MOD_PRODUCTION;
    prime_search_limit = ((1 << P_Q_BIAS_BITS) * (odd_mod));

    /**
     * Calculate the initial ERRK P & Q values, ensuring that they
     * are odd (3 mod 4)
     *
     *  Y2 = sha256(IMS[0:31] xor copy(0x5a, 32))  // (provided)
     *  Z3 = sha256(Y2 || copy(0x03, 32))
     *  ERRK_P[0:31]   = sha256(Z3 || copy(0x01, 32))
     *  ERRK_P[32:63]  = sha256(Z3 || copy(0x02, 32))
     *  ERRK_P[64:95]  = sha256(Z3 || copy(0x03, 32))
     *  ERRK_P[96:127] = sha256(Z3 || copy(0x41, 32))
     *  ERRK_Q[0:31]   = sha256(Z3 || copy(0x05, 32))
     *  ERRK_Q[32:63]  = sha256(Z3 || copy(0x06, 32))
     *  ERRK_Q[64:95]  = sha256(Z3 || copy(0x07, 32))
     *  ERRK_Q[96:127] = sha256(Z3 || copy(0x8, 32))
     *  ERRK_P[0] |= 0x03    // force P, Q to be odd
     *  ERRK_Q[0] |= 0x03
     *    :
     */
    calc_errk_pq_bias_odd(y2, ims, &errk_p, &errk_q, ims_sample_compatibility);

    /* Convert P & Q into FFs for arithmetic operations */
    if (ims_sample_compatibility) {
        /* Used in first 100 IMS samples */
        ff_from_big_endian_octet(p_ff, &errk_p, MCL_HFLEN);
        ff_from_big_endian_octet(q_ff, &errk_q, MCL_HFLEN);
    } else {
        /* Used subsequent to the first 100 IMS samples */
        ff_from_little_endian_octet(p_ff, &errk_p, MCL_HFLEN);
        ff_from_little_endian_octet(q_ff, &errk_q, MCL_HFLEN);
    }

    /**
     *    :
     *  <ensure ERRK_P & ERRK_Q are within 8192 of being prime>
     *    :
     *
     * Bias ERRK_P & ERRK_Q separately so that they are both prime.
     * We do this by testing them for primality, and if they aren't, then add
     * 2 and test again. Give up when we've swept all 4k possibilities for each
     * without finding a prime number.
     */
    MCL_FF_copy_C25519(priv_key.p, p_ff, MCL_HFLEN);

    for (p_bias = 0;
         p_bias < prime_search_limit;
         p_bias += odd_mod,
         MCL_FF_inc_C25519(priv_key.p, odd_mod, MCL_HFLEN)) {
        if (MCL_FF_comp_C25519(priv_key.p, errk_max_pq_ff, MCL_HFLEN) == 1) {
            /* The sum of P + P_bias will overflow */
            fprintf(stderr, "P would overflow - discard IMS\n");
            break;
        }
        /* Check if P is prime */
        if (MCL_FF_prime_C25519(priv_key.p, &rng, MCL_HFLEN) == 1) {
#ifdef RSA_PQ_FACTORABILITY
            if (ims_sample_compatibility) {
                MCL_FF_copy_C25519(p1, priv_key.p, MCL_HFLEN);
                MCL_FF_dec_C25519(p1, 1, MCL_HFLEN);

                if (MCL_FF_cfactor_C25519(p1, ERPK_EXPONENT, MCL_HFLEN)) {
                    printf("P continue\n");
                    continue;
                }
            }
#endif
           /*
             * Always start with the base value of Q, since the inner loop
             * modifies it.
             */
            MCL_FF_copy_C25519(priv_key.q, q_ff, MCL_HFLEN);

            for (q_bias = 0;
                 q_bias < prime_search_limit;
                 q_bias += odd_mod,
                 MCL_FF_inc_C25519(priv_key.q, odd_mod, MCL_HFLEN)) {
                if (MCL_FF_comp_C25519(priv_key.q, errk_max_pq_ff, MCL_HFLEN) == 1) {
                    /* The sum of Q + Q_bias will overflow */
                    fprintf(stderr, "Q would overflow - discard IMS\n");
                    break;
                }
                /**
                 * P_bias and Q_bias are guaranteed to be >= 0, < 8192, and even.
                 * Divide them by two to get a 12-bit version (< 4096) and pack
                 * them as a 24-bit number as they will be in IMS[32:34]
                 */
                pq_bias = (4096 * (p_bias / odd_mod)) + (q_bias / odd_mod);

#ifdef IMS_PQ_BIAS_HAMMING_BALANCED
                /**
                 * If enabled, reject if PQ bias doesn't have equal numbers
                 * of 1s and 0s.
                 */
                if (hamming_weight((uint8_t *)&pq_bias, 3) !=
                    IMS_PQ_BIAS_HAMMING_WEIGHT) {
                    continue;
                }
#endif
                /* Check if Q is prime */
                if (MCL_FF_prime_C25519(priv_key.q, &rng, MCL_HFLEN) == 1) {
#ifdef RSA_PQ_FACTORABILITY
                    if (ims_sample_compatibility) {
                        MCL_FF_copy_C25519(q1, priv_key.q, MCL_HFLEN);
                        MCL_FF_dec_C25519(q1, 1, MCL_HFLEN);

                        if (MCL_FF_cfactor_C25519(q1, ERPK_EXPONENT, MCL_HFLEN)) {
                            printf("Q continue\n");
                            continue;
                        }
                    }
#endif
                    goto SUCCESS;
                }
            }
        }
        /* If the Q loop ends, no Q + Q_bias was prime, so try next P */
    }
    /**
     * No valid P_bias and Q_bias combo was found within 8192, discard this
     *  IMS and try again
     */
    return EOVERFLOW;

SUCCESS:
    /**
     * When we get here, P and Q are already modified by the bias values
     * and are guaranteed to be prime.
     */

    /* Save the bias offset in IMS[32:34] */
    ims[32] = (uint8_t)(pq_bias);
    ims[33] = (uint8_t)(pq_bias >> 8);
    ims[34] = (uint8_t)(pq_bias >> 16);

    /**
     * Generate the public and private keys
     *    :
     *  ERPK_MOD = ERRK_Q * ERRK_P                 // generate modulus for ERPK
     *  ERPK_E = 65537                             // public exponent
     *  ERRK_D = RSA_secret(ERRK_P, ERK_Q, ERPK_E) // Private decrypt exponent
     *                                             // (unused in IMS creation)
     * On return, the structs are set to the following:
     *   - pub_key.n    ERRPK_MOD (p * q)
     *   - pub_key.e    ERPK_EXPONENT (65537)
     *
     *   - priv_key.p   secret prime p
     *   - priv_key.q   secret prime q
     *   - priv_key.dp  decrypting exponent mod (p-1)
     *   - priv_key.dq  decrypting exponent mod (q-1)
     *   - priv_key.c   1/p mod q
     */
    rsa_secret(&priv_key, &pub_key, ERPK_EXPONENT, ims_sample_compatibility);

    /* Convert the calculated FF nums back into octets for later storage */
    MCL_FF_toOctet_C25519(erpk_mod, pub_key.n, MCL_FFLEN);

    return status;
}


/**
 * @brief Generate an IMS value
 *
 * Generates a unique IMS value, storing it in the IMS output file and
 * the generated EPVK, ERPK and ESVK keys in the key
 * database.
 *
 * @param ims_sample_compatibility If true, generate IMS values that are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, generate the IMS value using
 *        the correct form.
 *
 * @returns Zero if successful, errno otherwise.
 */
int ims_generate(bool ims_sample_compatibility) {
    int status = 0;
    int epvk_status;
    int esvk_status;

    /* Generate a cryptographiclly good IMS value */
    do {
        /* Find a unique IMS value */
        do {
            ims_generate_candidate(ims);
            calculate_epuid_es3(ims, &ep_uid);
         } while (db_ep_uid_exists(&ep_uid));

        /* Calculate "Y2", used in generating EPSK, MPDK, ERRK, EPCK, ERGS */
        calculate_y2(ims, y2);

        /**
         * Calculate ERRK from that IMS (returns EOVERFLOW if we need to spin
         * a new IMS)
         */
        status = calc_errk(y2, ims, &erpk_mod, &errk_d,
                           ims_sample_compatibility);

        if (status == 0) {
            /* Calculate EPSK/EPVK and  ESSK/ESVK from the confirmed-valid IMS */
            calc_epsk(y2, &epsk);
            epvk_status = calc_epvk(&epsk, &epvk);
            calc_essk(y2, &essk);
            esvk_status = calc_esvk(&essk, &esvk);
            /**
             * For the first 100 samples, we didn't check epvk or esvk
             * generation status. In a production environment, we do, and
             * if either one fails, we discard the IMS.
             */
            if (!ims_sample_compatibility &&
                    ((epvk_status != 0) || (esvk_status != 0))) {
                status = -1;
            }
        }
    } while (status != 0);

    /**
     * Write the IMS value to the IMS file and the various keys and magic
     * numbers to the database
     */
    if (status == 0){
        status = ims_write(fp_ims, ims);
    }
    if (status == 0){
        status = db_add_keyset(&ep_uid, &epvk, &esvk, &erpk_mod);
    }

    return status;
}
