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

/**
 * If enabled, the following define will check RSA coefficients P & Q
 * for factorability by the ERPK exponent ("e" or 65537). This is used
 * by MIRACL in their MCL_RSA_KEY_PAIR function and was not in the
 * ERRK_P + ERRK_Q calculations specified.
 */
#define RSA_PQ_FACTORABILITY

/**
 * If enabled, the following define adds the constraint that the encoded
 * RSA P+Q bias values in IMS[32:34] must have an equal number of 1 and
 * zero bits (i.e., a Hamming weight of 12 - see IMS_PQ_BIAS_HAMMING_WEIGHT
 * below).
 */
#define IMS_PQ_BIAS_HAMMING_BALANCED


/* MSb mask for a byte */
#define BYTE_MASK_MSB               0x80

/* The maximum number of bytes to read from a prng_seed_file */
#define DEFAULT_PRNG_SEED_LENGTH    128

/* The PRNG seed is stored in this buffer */
uint8_t  prng_seed_buffer[EVP_MAX_MD_SIZE];
uint32_t prng_seed_length;

/* Cryptographically Secure Random Number Generator */
csprng  rng;        /* Generic */


/* IMS working set */
#define IMS_SIZE                    35
#define IMS_HAMMING_SIZE            32
#define IMS_PQ_BIAS_SIZE            (IMS_SIZE - IMS_HAMMING_SIZE)
#define IMS_HAMMING_WEIGHT          (IMS_HAMMING_SIZE * 8 / 2)
#define IMS_PQ_BIAS_HAMMING_WEIGHT  (IMS_PQ_BIAS_SIZE * 8 / 2)
uint8_t  ims[IMS_SIZE];

/**
 * 24-bit P&Q bias field, divided evenly into 12 bit fields for each of P & Q bias
 */
#define IMS_BIAS_BITS       ((IMS_SIZE - IMS_HAMMING_SIZE) * 8)
#define P_Q_BIAS_BITS       (IMS_BIAS_BITS / 2)

/**
 * When searching for P & Q primality, ensure it is odd by ORing the bottom
 * (ODD_MOD - 1) bits with 1, and stepping by ODD_MOD. Because we force the
 * bottom bits to be 1, we discount them from the bias, increasing our step
 * size and search limit.
 */
#define ODD_MOD_SAMPLE              2
#define ODD_MOD_PRODUCTION          4
#define ODD_MOD_BITMASK(odd_mod)    ((odd_mod) - 1)

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
 * @brief Perform any common IMS initialization
 *
 * @param prng_seed_file Filename from which to read the seed
 * @param prng_seed_string Raw seed string
 *
 * @returns Zero if successful, errno otherwise.
 */
int ims_common_init(const char * prng_seed_file,
                    const char * prng_seed_string);


/**
 * @brief Perform any common IMS de-initialization
 */
void ims_common_deinit(void);


void MCL_FF_fromOctetRev(mcl_chunk x[][MCL_BS],mcl_octet *S,int n);


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
 * @brief Calculate the Endpoint Primary Verification Key (EPVK)
 *
 * @param epsk A pointer to the input EPSK variable
 * @param epvk A pointer to the output EPVK variable
 *
 * @returns Zero if successful, MIRACL status otherwise (EPVK didn't validate)
 */
int calc_epvk(mcl_octet * epsk, mcl_octet * epvk);


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
 * @brief Calculate the Endpoint Secondary Verification Key (ESVK)
 *
 * @param essk A pointer to the input ESSK variable
 * @param esvk A pointer to the output ESVK variable
 *
 * @returns Zero if successful, MIRACL status otherwise (ESVK didn't validate)
 */
int calc_esvk(mcl_octet * essk, mcl_octet * esvk);


/**
 * @brief Calculate the Endpoint Rsa pRivate Key (ERRK) P & Q factors
 *
 * Calculates the ERRK_P & ERRK_Q, up to and including the bias-to-odd
 * step (i.e., that which can be extracted from IMS[0:31]).
 *
 *
 * @param y2 A pointer to the Y2 term used by all
 * @param ims A pointer to the ims (the upper 3 bytes will be modified)
 * @param erpk_p A pointer to a buffer to store the ERRK P coefficient
 * @param errk_q A pointer to a buffer to store the ERPK Q coefficient
 * @param ims_sample_compatibility If true, generate IMS values that are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, generate the IMS value using
 *        the correct form.
 *
 * @returns Zero if successful, errno otherwise.
 */
void calc_errk_pq_bias_odd(uint8_t * y2,
                           uint8_t * ims,
                           mcl_octet * errk_p,
                           mcl_octet * errk_q,
                           bool ims_sample_compatibility);


/**
 * @brief Convert a big-endian octet into an FF
 *
 * @param ff The output ff
 * @param octet The input octet
 * @param n size of FF in MCL_BIGs
 */
void ff_from_big_endian_octet(mcl_chunk ff[][MCL_BS],
                              mcl_octet * octet,
                              int n);


/**
 * @brief Reverse the byte order of a buffer
 *
 * @param buf
 * @param length
 */
void reverse_buf(uint8_t *buf, size_t length);


/**
 * @brief Convert a little-endian octet into an FF
 *
 * @param ff The output ff
 * @param octet The input octet
 * @param n size of FF in MCL_BIGs
 */
void ff_from_little_endian_octet(mcl_chunk ff[][MCL_BS],
                                 mcl_octet * octet,
                                 int n);

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
 * @param ims_sample_compatibility If true, generate IMS values that are
 *        compatible with the original (incorrect) 100 sample values sent
 *        to Toshiba 2016/01/14. If false, generate the IMS value using
 *        the correct form.
 */
void rsa_secret(MCL_rsa_private_key *PRIV,
                MCL_rsa_public_key *PUB,
                sign32 e,
                bool ims_sample_compatibility);


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
