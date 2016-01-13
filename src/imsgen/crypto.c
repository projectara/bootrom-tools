/**
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

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "crypto.h"

/**
 * (Yes it's uncommon to include a C file, but this is how MIRACL provided
 * a wrapper). The SHA256 wrappers provided/used are:
 *      shs256_init (Initialize the SHA hash)
 *      shs256_process (Add data to the SHA hash)
 *      shs256_hash (Finalize the SHA hash and return the digest)
 */
#include "../vendors/MIRACL/bootrom.c"

static sha256 shctx;


/**
 * @brief Initialize the SHA hash
 */
void hash_start(void) {
    shs256_init(&shctx);
}


/**
 * @brief Add data to the SHA hash
 *
 * @param data Pointer to the run of data to add to the hash.
 * @param datalen The length in bytes of the data run.
 */
void hash_update(const uint8_t *data, const size_t datalen) {
    size_t i;

    for (i = 0; i < datalen; i++) {
        shs256_process(&shctx, data[i]);
    }
}


/**
 * @brief Finalize the SHA hash and generate the digest
 *
 * @param digest Pointer to the digest buffer
 */
void hash_final(uint8_t * digest) {
    shs256_hash(&shctx,(char*)digest);
}


/**
 * @brief Perform a SHA hash and generate the digest
 *
 * This combines hash_init, hash_update and has_final into a single call for
 * those times when you only have a single blob to hash.
 *
 * @param data Pointer to the run of data to add to the hash.
 * @param datalen The length in bytes of the data run.
 * @param digest Pointer to the output digest buffer
 */
void hash_it(const uint8_t *data, const size_t datalen, uint8_t * digest) {
    hash_start();
    hash_update(data, datalen);
    hash_final(digest);
}
