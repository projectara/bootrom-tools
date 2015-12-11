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
 * This file contains common code for S2FL config data handiling
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <openssl/pem.h>
#include "util.h"
#include "s2config.h"

static bool is_secondstage_cfgdata_valid(secondstage_cfgdata *config) {
    if (config == NULL) {
        fprintf(stderr, "Invalid secondstage_cfgdata pointer\n");
        return false;
    }

    if (memcmp(config->sentinel,
               secondstage_cfg_sentinel,
               SECONDSTAGE_CFG_SENTINEL_SIZE)) {
        fprintf(stderr, "Invalid config data sentinel value\n");
        return false;
    }

    return true;
}

static bool is_config_valid(s2config_descriptor *cfg) {
    if (cfg == NULL) {
        fprintf(stderr, "Invalid config data pointer\n");
        return false;
    }

    return is_secondstage_cfgdata_valid(cfg->config);
}

s2config_descriptor *new_s2config(void) {
    s2config_descriptor *cfg =
        (s2config_descriptor *)malloc(sizeof(s2config_descriptor));

    if (cfg == NULL) {
        fprintf(stderr, "Failed to allocate memeory for config data\n");
        goto errorReturn;
    }

    memset(cfg, 0, sizeof(s2config_descriptor));

    cfg->config = (secondstage_cfgdata *)malloc(sizeof(secondstage_cfgdata));
    if (cfg->config == NULL) {
        fprintf(stderr, "Failed to allocate memeory for config data\n");
        goto errorReturn;
    }

    memset(cfg->config, 0, sizeof(secondstage_cfgdata));
    memcpy(cfg->config->sentinel,
           secondstage_cfg_sentinel,
           SECONDSTAGE_CFG_SENTINEL_SIZE);

    return cfg;
errorReturn:
    free_s2config(cfg);
    return NULL;
}

s2config_descriptor *copy_s2config(secondstage_cfgdata *config) {
    s2config_descriptor *cfg;
    int status = -1;

    if (!is_secondstage_cfgdata_valid(config)) {
        return NULL;
    }

    size_t size = sizeof(secondstage_cfgdata);
    size += config->number_of_public_keys * sizeof(crypto_public_key);

    cfg = (s2config_descriptor *)malloc(sizeof(s2config_descriptor));

    if (cfg == NULL) {
        fprintf(stderr, "Failed to allocate memeory for config data\n");
        goto errorReturn;
    }

    memset(cfg, 0, sizeof(s2config_descriptor));

    cfg->config = (secondstage_cfgdata *)malloc(size);
    if (cfg->config == NULL) {
        fprintf(stderr, "Failed to allocate memeory for config data\n");
        goto errorReturn;
    }

    memcpy(cfg->config, config, size);
    status = 0;
errorReturn:
    if (status) {
        free_s2config(cfg);
        cfg = NULL;
    }
    return cfg;
}

void free_s2config(s2config_descriptor *cfg) {
    if (cfg != NULL) {
        if (cfg->config) {
            free(cfg->config);
        }

        free(cfg);
    }
}

static int expand_config_for_more_pubkey(s2config_descriptor *cfg,
        int num_of_keys) {
    uint32_t oldsize = sizeof(secondstage_cfgdata) +
                       (cfg->config->number_of_public_keys *
                        sizeof(crypto_public_key));
    uint32_t newsize = oldsize + num_of_keys * sizeof(crypto_public_key);

    secondstage_cfgdata *newconfig;
    newconfig = (secondstage_cfgdata *)malloc(newsize);

    if (newconfig == NULL) {
        fprintf(stderr, "Failed to allocate memory for more public keys\n");
        return -1;
    }

    memset(newconfig, 0, newsize);
    memcpy(newconfig, cfg->config, oldsize);
    free(cfg->config);
    cfg->config = newconfig;

    return 0;
}

int s2config_add_public_key(s2config_descriptor *cfg,
                            char *pem_filename,
                            uint32_t key_type,
                            char *key_name) {
    FILE *fp;
    RSA *rsa_pub = NULL;
    int status = -1;

    if (!is_config_valid(cfg)) {
        return -1;
    }

    size_t key_name_size = sizeof(((crypto_public_key *)0)->key_name);
    if (strnlen(key_name, key_name_size) >= key_name_size) {
        fprintf(stderr, "key name is too long\n");
        return -1;
    }

    fp = fopen(pem_filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s\n", pem_filename);
        return -1;
    }

    ERR_load_crypto_strings();
    rsa_pub = PEM_read_RSA_PUBKEY(fp, NULL, NULL, NULL);
    if (rsa_pub == NULL) {
        fprintf(stderr, "Failed to read the public key\n");
        goto errorReturn;
    }

    uint32_t oldsize = cfg->config->number_of_public_keys;
    if (expand_config_for_more_pubkey(cfg, 1)) {
        goto errorReturn;
    }

    cfg->config->number_of_public_keys ++;

    crypto_public_key *key = &cfg->config->public_keys[oldsize];
    key->type = key_type;
    strcpy(key->key_name, key_name);

    BN_bn2bin(rsa_pub->n, key->key);

    status = 0;
errorReturn:
    if (rsa_pub) {
        RSA_free(rsa_pub);
    }
    fclose(fp);
    return status;
}

int s2config_set_fake_ims(s2config_descriptor *cfg, uint8_t *ims, size_t len) {
    if (!is_config_valid(cfg)) {
        return -1;
    }

    if (ims == NULL) {
        printf("NULL pointer for faked IMS\n");
        return -1;
    }

    if (len != FAKE_IMS_SIZE) {
        printf("Invalid IMS size\n");
        return -1;
    }

    if (is_constant_fill(ims, len, 0)) {
        cfg->config->use_fake_ims = 0;
    } else {
        cfg->config->use_fake_ims = 1;
        memcpy(cfg->config->fake_ims, ims, len);
    }

    return 0;
}

int s2config_set_fake_ara_vidpid(s2config_descriptor *cfg,
                                 uint32_t use_fake_ara_vidpid,
                                 uint32_t fake_ara_vid,
                                 uint32_t fake_ara_pid) {
    if (!is_config_valid(cfg)) {
        return -1;
    }

    cfg->config->use_fake_ara_vidpid = use_fake_ara_vidpid;
    cfg->config->fake_ara_vid = fake_ara_vid;
    cfg->config->fake_ara_pid = fake_ara_pid;
}

void print_s2config(s2config_descriptor *cfg) {
    int i, j;
    if (!is_config_valid(cfg)) {
        return;
    }

    printf("disable_jtag: %d\n",  cfg->config->disable_jtag);

    if (0 == cfg->config->use_fake_ara_vidpid) {
        printf("use_fake_ara_vidpid: false\n");
    }
    else {
        printf("use_fake_ara_vidpid: true. VID: 0x%08X PID: 0x%08X\n",
               cfg->config->fake_ara_vid,
               cfg->config->fake_ara_pid);
    }

    if (0 == cfg->config->use_fake_ims) {
        printf("use_fake_ims: false\n");
    }
    else {
        printf("use_fake_ims: true. IMS: \n\t");
        for (i = 0; i < FAKE_IMS_SIZE; i++) {
            printf("%02X ", cfg->config->fake_ims[i]);
            if ((i & 0xF) == 0xF) {
                printf("\n\t");
            }
        }
        printf("\n");
    }

    printf("number_of_public_keys: %d\n", cfg->config->number_of_public_keys);

    if (cfg->config->number_of_public_keys) {
        for (i = 0; i < cfg->config->number_of_public_keys; i++) {
            crypto_public_key *key = &cfg->config->public_keys[i];
            printf("public key %d:\n", i);
            printf("\ttype: %d\n", key->type);
            printf("\tname: %s\n\tkey:\t", key->key_name);
            for (j = 0; j < RSA2048_PUBLIC_KEY_SIZE; j++) {
                printf("%02X ", key->key[j]);
                if ((j & 0xF) == 0xF) {
                    printf("\n\t\t");
                }
            }
            printf("\n");
        }
    }
}

int save_s2config_to_file(char *filename, s2config_descriptor *cfg) {
    FILE *fp;
    int status = -1;

    if (!is_config_valid(cfg)) {
        return -1;
    }

    fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create file %s\n", filename);
        return -1;
    }

    size_t size = sizeof(secondstage_cfgdata);
    size += cfg->config->number_of_public_keys * sizeof(crypto_public_key);

    if (size != fwrite(cfg->config, 1, size, fp)) {
        fprintf(stderr, "Failed to write config data\n");
        goto errorReturn;
    }

    status = 0;
errorReturn:
    fclose(fp);
    return status;
}

s2config_descriptor *load_s2config_from_file(char *filename) {
    FILE *fp;
    int status = -1;
    s2config_descriptor *cfg;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        return NULL;
    }

    cfg = new_s2config();
    if (cfg == NULL) {
        goto errorReturn;
    }

    size_t size = sizeof(secondstage_cfgdata);
    if (size != fread(cfg->config, 1, size, fp)) {
        fprintf(stderr, "Failed to read config data\n");
        goto errorReturn;
    }

    if (!is_config_valid(cfg)) {
        goto errorReturn;
    }

    uint32_t num_keys = cfg->config->number_of_public_keys;
    if (num_keys) {
        size = num_keys * sizeof(crypto_public_key);
        if (expand_config_for_more_pubkey(cfg, num_keys)) {
            goto errorReturn;
        }

        if (size != fread(&cfg->config->public_keys[0], 1, size, fp)) {
            fprintf(stderr, "Failed to read config data\n");
            goto errorReturn;
        }
    }

    status = 0;
errorReturn:
    fclose(fp);
    if (status != 0) {
        free_s2config(cfg);
        cfg = NULL;
    }
    return cfg;
}

