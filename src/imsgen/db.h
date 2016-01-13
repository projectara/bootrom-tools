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

#ifndef _DATABASE_H
#define _DATABASE_H


/**
 * @brief Initialize the key database subsystem
 *
 * @param database_name The name of the key database
 *
 * @returns Zero if successful, errno otherwise.
 */
int db_init(const char * database_name);


/**
 * @brief De-initialize the key database subsystem
 *
 * Flushes the IMS output file, closes the database
 */
void db_deinit(void);


/**
 * @brief Determine if an EP_UID is already in the key database
 *
 * @param ep_uid The EndPoint Unique ID to check for
 *
 * @returns True if the EP_UID is already in the db, false if it isn't.
 */
bool db_ep_uid_exists(mcl_octet * ep_uid);


/**
 * @brief Save the Endpoint Primary Signing Key (EPSK) in the key database
 *
 * @param ep_uid The EndPoint Unique ID, used as a key
 * @param epsk The EPVK to save
 * @param essk The ESVK to save
 * @param erpk_mod The modulus for ERPK
 *
 * @returns Zero if successful, errno otherwise.
 */
int db_add_keyset(mcl_octet * ep_uid,
                  mcl_octet * epvk,
                  mcl_octet * esvk,
                  mcl_octet * erpk_mod);

#endif /* !_DATABASE_H */
