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

#ifndef _SIGN_H
#define _SIGN_H


/* Recognized key types (--type) */
#define KEY_TYPE_UNKNOWN    0
#define KEY_TYPE_S2FSK      1
#define KEY_TYPE_S3FSK      1

/* Recognized formats (--format) */
#define FORMAT_TYPE_UNKNOWN     0
#define FORMAT_TYPE_STANDARD    1
#define FORMAT_TYPE_ES3         2


extern parse_entry signature_algorithms[];
extern parse_entry package_types[];
extern parse_entry signature_formats[];


/**
 * @brief Sign a TFTF
 *
 * @param filename The pathname to the TFTF file to sign.
 * @param signature_format The pathname to the TFTF file to sign.
 * @param package_type The pathname to the TFTF file to sign.
 * @param signature_algorithm The pathname to the TFTF file to sign.
 * @param key_filename The pathname to the TFTF file to sign.
 * @param suffix The optional suffix to append to the name
 *
 * @returns True on success, false on failure
 */
bool sign_tftf(const char * filename,
               const int32_t signature_format,
               const uint32_t package_type,
               const uint32_t signature_algorithm,
               const char * key_filename,
               const char * suffix);

#endif /* _SIGN_H */
