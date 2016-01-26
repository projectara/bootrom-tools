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
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sqlite3.h>
#include "mcl_arch.h"
#include "mcl_oct.h"
#include "db.h"

/* Uncomment the following define to enable DB diagnostic messages */
/*#define DB_DEBUGMSG*/

#define HOLD_DB_OPEN

static sqlite3 *db;
static const char * insert_stmt =
    "INSERT INTO pub_keys(ep_uid, epvk, esvk, erpk_mod) VALUES (?, ?, ?, ?)";
static const char * select_format_stmt =
    "SELECT ep_uid, epvk, esvk, erpk_mod FROM pub_keys WHERE ep_uid = '%s'";


/**
 * @brief Initialize the key database subsystem
 *
 * @param database_name The name of the key database
 *
 * @returns Zero if successful, errno otherwise.
 */
int db_init(const char * db_name) {
    int status = 0;

    status = sqlite3_open(db_name, &db);
    if (status) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        db = NULL;
        status = ENOENT;
    }

    return status;
}


/**
 * @brief De-initialize the key database subsystem
 *
 * Flushes the IMS output file, closes the database
 */
void db_deinit(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}


/**
 * @brief Save the Endpoint Primary Verification Key (EPVK) in the key database
 *
 * @param ep_uid The EndPoint Unique ID, used as a key
 * @param epsk The EPSK to save
 * @param essk The ESSK to save
 * @param erpk_mod The modulus for ERPK to save
 *
 * @returns Zero if successful, errno otherwise.
 */
int db_add_keyset(mcl_octet * ep_uid,
                  mcl_octet * epvk,
                  mcl_octet * esvk,
                  mcl_octet * erpk_mod) {
    int status = 0;
    char ep_uid_hex[32];
    sqlite3_stmt *stmt;

#ifdef DB_DEBUGMSG
    printf("db_add_keyset:\n");
    display_binary_data(ep_uid->val, ep_uid->len, true, "ep_uid   ");
    display_binary_data(epvk->val, epvk->len, true, "epvk     ");
    display_binary_data(esvk->val, esvk->len, true, "esvk     ");
    display_binary_data(erpk_mod->val, erpk_mod->len, true, "erpk_mod ");
#endif
    status = sqlite3_prepare_v2(db, insert_stmt, -1, &stmt, NULL);
    if (status != SQLITE_OK) {
        fprintf(stderr, "db_add_keyset: prepare failed: %s\n",
                sqlite3_errmsg(db));
    } else {
        /* Bind the values to the statement */
        MCL_OCT_toHex(ep_uid, ep_uid_hex);
        status = sqlite3_bind_text(stmt, 1, ep_uid_hex, 16, SQLITE_STATIC);
        if (status != SQLITE_OK) {
            fprintf(stderr, "db_add_keyset: ep_uid bind failed: %s\n",
                    sqlite3_errmsg(db));
        } else {
            status = sqlite3_bind_blob(stmt, 2, epvk->val, epvk->len,
                                       SQLITE_STATIC);
            if (status != SQLITE_OK) {
                fprintf(stderr, "db_add_keyset: epvk bind failed: %s\n",
                        sqlite3_errmsg(db));
            } else {
                status = sqlite3_bind_blob(stmt, 3, esvk->val, esvk->len,
                                           SQLITE_STATIC);
                if (status != SQLITE_OK) {
                    fprintf(stderr, "db_add_keyset: esvk bind failed: %s\n",
                            sqlite3_errmsg(db));
                } else {
                    status = sqlite3_bind_blob(stmt, 4, erpk_mod->val,
                                               erpk_mod->len, SQLITE_STATIC);
                    if (status != SQLITE_OK) {
                        fprintf(stderr,
                                "db_add_keyset: erpk_mod bind failed: %s\n",
                                sqlite3_errmsg(db));
                    }
                }
            }
        }


        /* Push the row out to the db */
        status = sqlite3_step(stmt);
        if (status != SQLITE_DONE) {
            fprintf(stderr, "db_add_keyset: can't add row: %s\n",
                    sqlite3_errmsg(db));
        }

        status = sqlite3_finalize(stmt);
    }

    return status;
}


/**
 * @brief Fetch a blob into an octet
 *
 * @param stmt The active query
 * @param column Which column to read as a blob
 * @param octet The mcl_octet in which to store the blob
 *
 * @returns true if successful, false otherwise.
 */
static int db_get_blob(sqlite3_stmt *stmt,
                       int column,
                       mcl_octet * octet) {
    int length;
    bool success = false;

    /* Fetch the desired column */
    if (stmt && octet) {
        length = sqlite3_column_bytes(stmt, column);
        if (length <= octet->max) {
            memcpy(octet->val, sqlite3_column_blob(stmt, column), length);
            octet->len = length;
            success = true;
        } else {
            /* Octet buffer too small */
            fprintf(stderr, "db_get_blob: octet too small (%d vs %d):\n", octet->max, length);
        }
    }

    return success;
}


/**
 * @brief Fetch the set of keys associated with an EP_UID
 *
 * @param ep_uid The EndPoint Unique ID, to look up
 * @param epsk (Optional) the EPSK to retrieve
 * @param essk (Optional) the ESSK to retrieve
 * @param erpk_mod (Optional) the modulus for ERPK to retrieve
 *
 * @returns Zero if successful, errno otherwise.
 */
int db_get_keyset(mcl_octet * ep_uid,
                  mcl_octet * epvk,
                  mcl_octet * esvk,
                  mcl_octet * erpk_mod) {
    int status = 0;
    int step = 0;
    char ep_uid_hex[32];
    char query[256];
    sqlite3_stmt *stmt;

    /* Convert the EP_UID to a hex string to use as the key */
    MCL_OCT_toHex(ep_uid, ep_uid_hex);
    snprintf(query, sizeof(query), select_format_stmt, ep_uid_hex);
    status = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (status != SQLITE_OK) {
        fprintf(stderr, "db_get_keyset: prepare failed: %s\n", sqlite3_errmsg(db));
    } else {
        /* Search the result for the ep_uid */
        step = sqlite3_step(stmt);
        if (step == SQLITE_ROW) {
#ifdef DB_DEBUGMSG
            printf("db_get_keyset:\n");
            display_binary_data(ep_uid->val, ep_uid->len, true, "ep_uid   ");
#endif
            /* fetch the desired columns */
            if (epvk) {
                if (db_get_blob(stmt, 1, epvk)) {
#ifdef DB_DEBUGMSG
                    display_binary_data(epvk->val, epvk->len, true, "epvk     ");
#endif
                }
            }
            if (esvk) {
                if (db_get_blob(stmt, 2, esvk)) {
#ifdef DB_DEBUGMSG
                    display_binary_data(esvk->val, esvk->len, true, "esvk     ");
#endif
                }
            }
            if (erpk_mod) {
                if (db_get_blob(stmt, 3, erpk_mod)) {
#ifdef DB_DEBUGMSG
                    display_binary_data(erpk_mod->val, erpk_mod->len, true, "erpk_mod ");
#endif
                }
            }
        }

        status = sqlite3_finalize(stmt);
    }

    if (step != SQLITE_ROW) {
        status = SQLITE_NOTFOUND;
    }
    return status;
}


/**
 * @brief Determine if an EP_UID is already in the key database
 *
 * @param ep_uid The EndPoint Unique ID to check for
 *
 * @returns True if the EP_UID is already in the db, false if it isn't.
 */
bool db_ep_uid_exists(mcl_octet * ep_uid) {
    int status = 0;
    char ep_uid_hex[32];
    char sql_stmt[256];
    sqlite3_stmt *stmt;
    bool ep_uid_exists = false;


   return (db_get_keyset(ep_uid, NULL, NULL, NULL) != SQLITE_NOTFOUND);
}
