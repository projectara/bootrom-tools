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
#include "ims_test.h"

/* Uncomment the following define to enable IMS diagnostic messages */
/*#define IMS_DEBUGMSG*/


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
    int odd_mod;
    uint8_t odd_mod_bitmask;

    odd_mod = (ims_sample_compatibility)? ODD_MOD_SAMPLE : ODD_MOD_PRODUCTION;

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
     *    :
     */
    calc_errk_pq_bias_odd(y2, ims, &errk_p, &errk_q, ims_sample_compatibility);


    /* Convert P & Q to FF format */
    if (ims_sample_compatibility) {
        /* Used in first 100 IMS samples */
        ff_from_big_endian_octet(p_ff, &errk_p, MCL_HFLEN);
        ff_from_big_endian_octet(q_ff, &errk_q, MCL_HFLEN);
    } else {
        /* Used subsequent to the first 100 IMS samples */
        ff_from_little_endian_octet(p_ff, &errk_p, MCL_HFLEN);
        ff_from_little_endian_octet(q_ff, &errk_q, MCL_HFLEN);
    }


    /* Extract the P & Q bias from IMS[32:34] */
    pq_bias = ims[32] | (ims[33] << 8) | (ims[34] << 16);
    p_bias = ((pq_bias / 4096) * odd_mod);
    q_bias = ((pq_bias % 4096) * odd_mod);


    /* Bias P & Q */
    MCL_FF_inc_C25519(p_ff, p_bias, MCL_HFLEN);
    MCL_FF_inc_C25519(q_ff, q_bias, MCL_HFLEN);

    /**
     * Generate the public and private exponents
     *   :
     *  ERPK_MOD = ERRK_Q * ERRK_P                 // generate modulus for ERPK
     *  ERPK_E = 65537                             // public exponent
     *  ERRK_D = RSA_secret(ERRK_P, ERK_Q, ERPK_E) // Private decrypt exponent
     *
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
    rsa_secret(&rsa_private, &rsa_public, ERPK_EXPONENT, ims_sample_compatibility);

    /* Convert the calculated FF nums back into octets for later use */
    MCL_FF_toOctet_C25519(erpk_mod, rsa_public.n, MCL_FFLEN);

    return status;
}


/**
 * @brief Calculate the EP_UID and our keys.
 *
 * @param ims A pointer to the IMS
 * @param ims_sample_compatibility If true, extracted keys are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, extracted keys use
 *        the correct form.
 *
 * @returns Zero if successful, errno otherwise.
 */
void calculate_keys(uint8_t ims[IMS_SIZE],
                    bool ims_sample_compatibility) {

    /**
     * Calculate the Endpoint Unique ID (EP_UID) from the IMS and the
     * private/public keys.
     */
    calculate_epuid_es3(ims, &ep_uid);
    ep_uid.len = EP_UID_SIZE;

    /* Calculate "Y2", used in generating EPSK, MPDK, ERRK, EPCK, ERGS */
    calculate_y2(ims, y2);

    /* Calculate ERRK/ERPK, EPSK/EPVK and  ESSK/ESVK */
    calc_epsk(y2, &epsk);
    calc_epvk(&epsk, &epvk);
    calc_essk(y2, &essk, ims_sample_compatibility);
    calc_esvk(&essk, &esvk);
    calc_errk(y2, ims, &erpk_mod, &errk_d, ims_sample_compatibility);
}



/**
 * @brief Sign a message with the RSA key
 *
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_rsa.c
 *
 * @param message The message to sign
 * @param signature The generated signature (must point to a
 *        MCL_RFS-byte buffer)
 *
 * @returns Zero if successful, -1 otherwise.
 */
int rsa_sign_message(mcl_octet * message,
                     mcl_octet * signature) {
    int status = 0;
    static char c[MCL_RFS];
    mcl_octet C={0,sizeof(c),c};

    /* PKCS V1.5 padding of a message prior to RSA signature M -> C */
    if (MCL_PKCS15_RSA2048(MCL_HASH_TYPE_RSA, message, &C) != 1) {
        fprintf(stderr, "Unable to pad message prior to RSA signature\n");
        status = -1;
    } else {
        /* create signature C -> S */
        MCL_RSA_DECRYPT_RSA2048(&rsa_private, &C, signature);
    }

    return status;
}


/**
 * @brief Verify a message signed with the RSA key
 *
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_rsa.c
 *
 * @param message The message to verify
 * @param signature The signature for the message
 *
 * @returns Zero if successful, -1 otherwise.
 */
int rsa_verify_message(mcl_octet * message,
                       mcl_octet * signature) {
    int status = 0;
    static char c[MCL_RFS];
    static char ml[MCL_RFS];
    mcl_octet ML={0, sizeof(ml), ml};
    mcl_octet C={0,sizeof(c),c};

    /* We need to performe the same PKCS V1.5 padding that was applied to
     * the plaintext before it was signed M -> C */
    if (MCL_PKCS15_RSA2048(MCL_HASH_TYPE_RSA, message, &C) != 1) {
        fprintf(stderr, "Unable to pad message prior to RSA verification\n");
        status = -1;
    } else {
        /* Verify the signature */
        MCL_RSA_ENCRYPT_RSA2048(&rsa_public, signature, &ML);
        if (MCL_OCT_comp(&C, &ML)) {
            status = 0;
        } else {
            printf("Signature is INVALID:\r\n");
            MCL_OCT_output(signature);
            printf("\r\n");
            status = -1;
        }
    }

    return status;
}


/**
 * @brief Sign a message with the  primary/secondary ECC key
 *
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_ecdh.c
 *
 * @param message The message to sign
 * @param signature_c The generated signature (must point to an
 *        EPSK_SIZE-byte buffer for primary signing,  or an
 *        ESSK_SIZE-byte buffer for secondary signing)
 * @param signature_d The generated signature (must point to an
 *        EPSK_SIZE-byte buffer for primary signing,  or an
 *        ESSK_SIZE-byte buffer for secondary signing)
 * @param primary If true, use EPSK, if false, use ESSK
 *
 * @returns Zero if successful, -1 otherwise.
 */
int ecc_sign_message(mcl_octet * message,
             mcl_octet * signature_c,
             mcl_octet * signature_d,
             bool primary) {
    int status = 0;
    int mcl_status;
    int i;
    char * key_name = primary? "Primary" : "Secondary";

#if MCL_CURVETYPE!=MCL_MONTGOMERY
    /**
     * Sign the message M with private signing key epsk, outputting two
     * signature components: CS & DS
     */
    if (primary) {
        mcl_status = MCL_ECPSP_DSA_C448(MCL_HASH_TYPE_ECC,
                                        &rng,
                                        &epsk,
                                        message,
                                        signature_c,
                                        signature_d);
    } else {
        mcl_status = MCL_ECPSP_DSA_C25519(MCL_HASH_TYPE_ECC,
                                          &rng,
                                          &essk,
                                          message,
                                          signature_c,
                                          signature_d);

    }
    if (mcl_status !=0 ) {
        printf("ERROR: %s ECDSA Signature Failed\r\n", key_name);
        status = -1;
    }
#else
    status = -1;
#endif

    return status;
}


/**
 * @brief  Verify a message signed with the primary/secondary ECC key
 *
 * This code is borrowed from MIRACL's ...MIRACL/src/tests/test_ecdh.c
 *
 * @param message The message to verify
 * @param signature_c The generated signature (must point to an
 *        EPSK_SIZE-byte buffer for primary signing,  or an
 *        ESSK_SIZE-byte buffer for secondary signing)
 * @param signature_d The generated signature (must point to an
 *        EPSK_SIZE-byte buffer for primary signing,  or an
 *        ESSK_SIZE-byte buffer for secondary signing)
 * @param primary If true, use EPVK, if false, use ESVK
 *
 * @returns Zero if successful, -1 otherwise.
 */
int ecc_verify_message(mcl_octet * message,
               mcl_octet * signature_c,
               mcl_octet * signature_d,
               bool primary) {
    int status = 0;
    int mcl_status;
    int i;
    char * key_name = primary? "Primary" : "Secondary";
    /* NOTE: CS, DS need to be sized to max(EPSK_SIZE, ESSK_SIZE) */
    static char m[32];
    static char cs[128];
    static char ds[128];
    mcl_octet M={0,sizeof(m),m};
    mcl_octet CS={0,sizeof(cs),cs};
    mcl_octet DS={0,sizeof(ds),ds};

#if MCL_CURVETYPE!=MCL_MONTGOMERY
    /**
     * Verify the signature: verify message M with public verification key
     * epvk and the two components, CS & DS generated from signing M above.
     */
    if (primary) {
        mcl_status = MCL_ECPVP_DSA_C448(MCL_HASH_TYPE_ECC,
                                        &epvk,
                                        message,
                                        signature_c,
                                        signature_d);
    } else {
        mcl_status = MCL_ECPVP_DSA_C25519(MCL_HASH_TYPE_ECC,
                                          &esvk,
                                          message,
                                          signature_c,
                                          signature_d);
    }
    if (mcl_status != 0) {
      fprintf(stderr, "ERROR: %s ECC failed:\r\n",
              key_name);

      printf("Message = 0x");
      MCL_OCT_output(message);
      printf("\r\n");
      printf("Signature C = 0x");
      MCL_OCT_output(signature_c);
      printf("\r\n");
      printf("Signature D = 0x");
      MCL_OCT_output(signature_d);
      printf("\r\n");
      status = -1;
    } else {
      status = 0;
    }
#else
    status = -1;
#endif

    return status;
}


/**
 * @brief Generate a cryptographically good 32-bit random number
 *
 * @returns 32 wonderfully random bits
 */
uint32_t rand32(void) {
    uint32_t r;

    r = (MCL_RAND_byte(&rng) << 24) |
        (MCL_RAND_byte(&rng) << 16) |
        (MCL_RAND_byte(&rng) << 8) |
        MCL_RAND_byte(&rng);
}
