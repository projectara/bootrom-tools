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

#ifndef __INCLUDE_S2CONFIG_H
#define __INCLUDE_S2CONFIG_H

#include "2ndstage_cfgdata.h"

/**
 * @brief structure to hold the pointer and neccessary information of the
 *        second stage FW config data.
 *        Currently only contain the pointer. But it is still a good idea
 *        to use this instead of the secondstage_cfgdata pointer directly,
 *        since the config data may get moved (re-allocated) when more data
 *        is added (say adding public key data) at runtime.
 */
typedef struct {
    secondstage_cfgdata *config;
} s2config_descriptor;

/**
 * @brief allocate memory and initialize s2config_descriptor
 * @param none
 * @return pointer to a s2config_descriptor, which contain an initialized
 *         secondstage_cfgdata.
 *         NULL if failed.
 * @NOTE: memory allocated by new_s2config should be freed by free_s2config
 */
s2config_descriptor *new_s2config(void);

/**
 * @brief allocate memory and initialize s2config_descriptor from existing
 *        secondstage_cfgdata.
 * @param config secondstage_cfgdata to copy from.
 * @return pointer to a s2config_descriptor, which contain duplicated
 *         secondstage_cfgdata.
 *         NULL if failed.
 * @NOTE: memory allocated by copy_s2config should be freed by free_s2config
 */
s2config_descriptor *copy_s2config(secondstage_cfgdata *config);

/**
 * @brief free memory of s2config_descriptor and inner secondstage_cfgdata
 * @param cfg pointer to s2config_descriptor
 * @return none
 */
void free_s2config(s2config_descriptor *cfg);

/**
 * @brief add public key data to the config data
 * @param cfg s2config_descriptor pointer
 * @param pem_filename name of the public PEM key file.
 *        The public key file should have "-----BEGIN PUBLIC KEY-----" marker
 * @param key_type type of the key (eg. 1 for RSA2048_SHA256)
 * @param key_name name of the public key
 * @return 0 for success, <0 for error
 */
int s2config_add_public_key(s2config_descriptor *cfg,
                            char *pem_filename,
                            uint32_t key_type,
                            char *key_name);

/**
 * @brief set fake ara vid/pid to the config data
 * @param cfg s2config_descriptor pointer
 * @param use_fake_ara_vidpid 1 to use fake vid/pid, 0 for not use them
 * @param fake_ara_vid fake vid
 * @param fake_ara_pid fake pid
 * @return 0 for success, <0 for error
 */
int s2config_set_fake_ara_vidpid(s2config_descriptor *cfg,
                                 uint32_t use_fake_ara_vidpid,
                                 uint32_t fake_ara_vid,
                                 uint32_t fake_ara_pid);

/**
 * @brief set fake ims to the config data
 * @param cfg s2config_descriptor pointer
 * @param fake_ims fake ims
 * @return 0 for success, <0 for error
 */
int s2config_set_fake_ims(s2config_descriptor *cfg, uint8_t *ims, size_t len);

/**
 * @brief print out data in the config data
 * @param cfg config data to print
 * @return none
 */
void print_s2config(s2config_descriptor *cfg);

/**
 * @brief save config data to raw binary file, which can be build into TFTF
 * @param filename name of the output file
 * @param cfg config data to be saved
 * @return 0 for success, <0 for error
 */
int save_s2config_to_file(char *filename, s2config_descriptor *cfg);

/**
 * @brief load config data from raw binary file
 * @param filename name of the input file
 * @return memory allocated containing the config data from the input file
 *         NULL for failure.
 * @NOTE: memory allocated by load_s2config_from_file should
 *        be freed by free_s2config
 */
s2config_descriptor *load_s2config_from_file(char *filename);

#endif /* __INCLUDE_S2CONFIG_H */
