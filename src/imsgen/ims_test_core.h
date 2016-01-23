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

#ifndef _IMS_TEST_CORE_H
#define _IMS_TEST_CORE_H


/* Names of the EP_UID and signature files used by imstest1, imstest2 */
#define FNAME_EP_UID            "EP_UID.sig"
#define FNAME_RSA_SIG           "ERRK.sig"
#define FNAME_ECC_PRIMARY_SIG   "EPVK.sig"
#define FNAME_ECC_SECONDARY_SIG "ESVK.sig"


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
int calc_errk(uint8_t * y2,
              uint8_t * ims,
              mcl_octet * erpk_mod,
              mcl_octet * errk_d,
              bool ims_sample_compatibility);


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
                    bool ims_sample_compatibility);


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
                     mcl_octet * signature);


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
                       mcl_octet * signature);


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
             bool primary);


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
               bool primary);


/**
 * @brief Generate a cryptographically good 32-bit random number
 *
 * @returns 32 wonderfully random bits
 */
uint32_t rand32(void);

#endif /* !_IMS_TEST_CORE_H */
