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
 * @brief: This file contains the header information for the signing code
 * used by "sign-tftf".
 *
 */

#ifndef _IMS_COMMON_H
#define _IMS_COMMON_H

/* MSb mask for a byte */
#define BYTE_MASK_MSB               0x80

/* The maximum number of bytes to read from a prng_seed_file */
#define DEFAULT_PRNG_SEED_LENGTH    128

/* The PRNG seed is stored in this buffer */
uint8_t  prng_seed_buffer[EVP_MAX_MD_SIZE];
uint32_t prng_seed_length;

/* Cryptographically Secure Random Number Generator */
csprng   rng;


/* IMS working set */
#define IMS_SIZE            35
#define IMS_HAMMING_SIZE    32
#define IMS_HAMMING_WEIGHT  (IMS_HAMMING_SIZE * 8 / 2)
uint8_t  ims[IMS_SIZE];

/* Endpoint Unique ID (EP_UID) working set */
#define EP_UID_SIZE         8
mcl_octet ep_uid;


/* Hash value used in calculating EPSK, MPDK ERRK */
#define Y2_SIZE     SHA256_HASH_DIGEST_SIZE
uint8_t  y2[Y2_SIZE];

/* Scratch octet for generating terms to feed sha256 */
uint8_t  scratch_buf[128];
mcl_octet scratch;


/* Endpoint Primary Signing/Verification Keys (EPSK, EPVK) */
#define EPSK_SIZE       56
#define EPVK_SIZE       113
mcl_octet epsk;
mcl_octet epvk;


/* Endpoint Secondary Signing/Verification Keys (ESSK/ESVK) */
#define ESSK_SIZE       32
#define ESVK_SIZE       65
mcl_octet essk;
mcl_octet esvk;

/* Endpoint Rsa pRivate Key (ERRK/ERPK) */
#define ERRK_PQ_SIZE    128
mcl_octet errk_p;
mcl_octet errk_q;
mcl_octet erpk_mod;
mcl_octet errk_d;

/*
 * The number of BIG nums needed to represent P or Q as an FF num.
 * However, the ERPK_MOD is the product P * Q, and the FF multiplication
 * function needs all values to be the same length, we need to size the
 * FFs for this
 * */

#define ERPK_EXPONENT   65537
mcl_chunk p_ff[MCL_HFLEN][MCL_BS];
mcl_chunk q_ff[MCL_HFLEN][MCL_BS];

MCL_rsa_private_key rsa_private;
MCL_rsa_public_key  rsa_public;


/**
 * @brief Parse a raw seed string
 *
 * Allocates a buffer for the seed string and points the prng_seed mcl_octet
 * at it
 *
 * @param prng_seed_file Filename from which to read the seed
 * @param prng_seed_string Raw seed string
 *
 * @returns Zero if successful, errno otherwise.
 */
int get_prng_seed(const char * prng_seed_file,
                  const char * prng_seed_string);


/**
 * @brief Implement a canonical "X = sha256(Y || copy(b, n)" operation
 *
 * This combines hash_init, hash_update and has_final into a single call for
 * those times when you only have a single blob to hash.
 *
 * @param digest_x Pointer to the output digest buffer ("X" above)
 * @param hash_y Pointer to the input digest ("Y" above)
 * @param extend_byte The extension byte ("b" above)
 * @param extend_count The numnnber of exension bytes to concatenate ("n" above)
 */
void sha256_concat(uint8_t * digest_x,
                   uint8_t * hash_y,
                   const uint8_t extend_byte,
                   uint32_t extend_count);


/**
 * @brief Calculate the EP_UID from the IMS (ES3 version)
 *
 * A mistake in ES3 boot ROM makes the EPUID of ES3 different
 * than the spec says. We must use this for the foreseeable future
 *
 * @param ims_value A pointer to the 35-byte IMS value
 * @param ep_uid A pointer to the octet EP_UID output value
 */
void calculate_epuid_es3(uint8_t * ims_value,
                         mcl_octet * ep_uid);


/**
 * @brief Calculate the EP_UID from the IMS (correct version)
 *
 * This is the EP_UID calculation, correctly implemented - usable
 * on post-ES3 chips
 *
 * @param ims_value A pointer to the 35-byte IMS value
 * @param ep_uid A pointer to the octet EP_UID output value
 */
void calculate_epuid(uint8_t * ims_value,
                     mcl_octet * ep_uid);


/**
 * @brief Calculate the "Y2" value
 *
 * "Y2" is a term used in all of the subsequent key generation routines.
 *
 * @param ims_value A pointer to the 35-byte IMS value
 * @param y2 A pointer to the 8-byte "Y2 output value
 */
void calculate_y2(uint8_t * ims_value, uint8_t * y2);


/**
 * @brief Calculate the EPSK and EPVK
 *
 * Calculate the Endpoint Primary Signing Keys (EPSK)
 *
 * @param y2 A pointer to the Y2 term used by all
 * @param epsk A pointer to the output EPSK variable
 */
void calc_epsk(uint8_t * y2, mcl_octet * epsk);


/**
 * @brief Calculate the EPSK and EPVK
 *
 * Calculate the Endpoint Primary Verification Key (EPVK)
 *
 * @param epsk A pointer to the input EPSK variable
 * @param epsk A pointer to the output EPVK variable
 */
void calc_epvk(mcl_octet * epsk, mcl_octet * epvk);


/**
 * @brief Calculate the ESSK
 *
 * Calculate the Endpoint Secondary Signing Key (ESSK)
 *
 * @param y2 A pointer to the Y2 term used by all
 * @param essk A pointer to the output ESSK variable
 */
void calc_essk(uint8_t * y2, mcl_octet * essk);


/**
 * @brief Calculate the ESVK
 *
 * Calculate the Endpoint Secondary Verification Key (ESVK)
 *
 * @param y2 A pointer to the Y2 term used by all
 * @param essk A pointer to the input ESSK variable
 * @param essk A pointer to the output ESVK variable
 */
void calc_esvk(mcl_octet * essk, mcl_octet * esvk);


/**
 * @brief Calculate the private decryption exponent
 *
 * NOTE: This is largely copied from MCL_RSA_KEY_PAIR and retains its syntax
 * for compatability. The plan-of-record is that MIRACL will provide their own
 * version of this function at a later date.
 *
 * @param PRIV Pointer to private key (P & Q must be filled in)
 * @param PUB Pointer to public key (blank)
 * @param e Constant public exponent FF instance
 */
void rsa_secret(MCL_rsa_private_key *PRIV,
                MCL_rsa_public_key *PUB,
                sign32 e);


/**
 * @brief Calculate the Endpoint Rsa pRivate Key (ERRK) P & Q factors
 *
 * Calculates the ERRK_P & ERRK_Q, without any of the bias calculations
 * (i.e., that which can be extracted from IMS[0:31]).
 *
 *
 * @param y2 A pointer to the Y2 term used by all
 * @param ims A pointer to the ims (the upper 3 bytes will be modified)
 * @param erpk_mod A pointer to a buffer to store the modulus for ERPK
 * @param errk_d A pointer to a buffer to store the  ERPK decryption exponent
 *
 * @returns Zero if successful, errno otherwise.
 */
void calc_errk_pq_unbiased(uint8_t * y2,
                           uint8_t * ims,
                           mcl_octet * erpk_p,
                           mcl_octet * errk_q);
/**
 * @brief print out an FF variable
 *
 * NOTE: This is largely copied from MCL_RSA_KEY_PAIR and retains its syntax
 * for compatability. The plan-of-record is that MIRACL will provide their own
 * version of this function at a later date.
 *
 * @param title Optional title
 * @param f The FF number to print
 * @param n @param n size of FF in MCL_BIGs
 */
void print_ff(char * title, mcl_chunk ff[][MCL_BS], int n);

#endif /* !_IMS_COMMON_H */
