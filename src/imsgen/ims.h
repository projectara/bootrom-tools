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

#ifndef _IMS_H
#define _IMS_H



/**
 * @brief Initialize the IMS generation subsystem
 *
 * Opens the database and IMS output file, seeds the PRNG, etc.
 *
 * @param prng_seed_file Filename from which to read the seed
 * @param prng_seed_string Raw seed string
 * @param ims_filename The name of the IMS output file
 * @param database_name The name of the certificate database
 *
 * @note One and only 1 of prng_seed_file and prng_seed_string must be used.
 *
 * @returns Zero if successful, errno otherwise.
 */
int ims_init(const char * prng_seed_file,
             const char * prng_seed_string,
             const char * ims_filename,
             const char * database_name);


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
int ims_generate(bool ims_sample_compatibility);


/**
 * @brief De-initialize the IMS generation subsystem
 *
 * Flushes the IMS output file, closes the database
 */
void ims_deinit(void);

#endif /* !_IMS_H */
