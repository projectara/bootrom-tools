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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include "parse_support.h"
#include "util.h"
#include "s2config.h"

uint32_t use_fake_vidpid = 0;
uint32_t fake_vid = 0;
uint32_t fake_pid = 0;
char *key_file_name = NULL;
uint32_t key_type = 0;
char *key_name = NULL;
char *output_filename = NULL;
uint8_t fake_ims[FAKE_IMS_SIZE] = {0};

bool cb_use_fake_vidpid(const int option, const char * optarg,
                        struct optionx * optx);
bool cb_fake_vid(const int option, const char * optarg,
                 struct optionx * optx);
bool cb_fake_pid(const int option, const char * optarg,
                 struct optionx * optx);
bool cb_fake_ims(const int option, const char * optarg,
                 struct optionx * optx);
bool cb_key_type(const int option, const char * optarg,
                 struct optionx * optx);

static char * use_fake_vidpid_names[] = { "use_fake_vidpid", NULL };
static char * fake_vid_names[] = { "fake_vid", NULL };
static char * fake_pid_names[] = { "fake_pid", NULL };
static char * fake_ims_names[] = { "fake_ims", NULL };
static char * key_name_names[] = { "key_name", NULL };
static char * key_type_names[] = { "key_type", NULL };
static char * key_file_name_names[] = { "public_key", NULL };
static char * output_names[] = { "output", NULL };

static struct optionx parse_table[] = {
    /* use fake ara vid/pid args */
    { 'd', use_fake_vidpid_names, "num", &use_fake_vidpid, 0,
      DEFAULT_VAL, &cb_use_fake_vidpid, 0,
      "use fake vid/pid or not"
    },
    { 'v', fake_vid_names, "num", &fake_vid, 0,
      DEFAULT_VAL, &cb_fake_vid, 0,
      "fake vid"
    },
    { 'p', fake_pid_names, "num", &fake_pid, 0,
      DEFAULT_VAL, &cb_fake_pid, 0,
      "fake pid"
    },
    { 'n', key_name_names, "text", &key_name, 0,
      DEFAULT_VAL, &store_str, 0,
      "key name"
    },
    { 't', key_type_names, "num", &key_type, 0,
      DEFAULT_VAL, &cb_key_type, 0,
      "key type"
    },
    { 'k', key_file_name_names, "text", &key_file_name, 0,
      DEFAULT_VAL, &store_str, 0,
      "key file name"
    },
    { 'o', output_names, "text", &output_filename, 0,
      REQUIRED, &store_str, 0,
      "output filename"
    },
    { 'i', fake_ims_names, "text", &fake_ims, 0,
      DEFAULT_VAL, &cb_fake_ims, 0,
      "fake ims"
    },
    { 0, NULL, NULL, NULL, 0, 0, NULL, 0 , NULL}
};

static char * all_args = "d:v:p:n:t:k:o";

static char * epilog =
    "NOTES:\n";

bool cb_use_fake_vidpid(const int option, const char * optarg,
                        struct optionx * optx) {
    bool success = true;
    success = get_num(optarg, (optx->long_names)[0], &use_fake_vidpid);
    return success;
}

bool cb_fake_vid(const int option, const char * optarg,
                 struct optionx * optx) {
    bool success = true;
    success = get_num(optarg, (optx->long_names)[0], &fake_vid);
    balance_vidpid(&fake_vid, "vid");
    return success;
}

bool cb_fake_pid(const int option, const char * optarg,
                 struct optionx * optx) {
    bool success = true;
    success = get_num(optarg, (optx->long_names)[0], &fake_pid);
    balance_vidpid(&fake_pid, "pid");
    return success;
}

bool cb_fake_ims(const int option, const char * optarg,
                 struct optionx * optx) {
    bool success = true;
    int i;
    char *cur;

    if (strlen(optarg)/2 != FAKE_IMS_SIZE) {
        printf("Invalid IMS size\n");
        return false;
    }

    cur = (char *)optarg;
    for (i = 0; i < FAKE_IMS_SIZE; i++) {
        sscanf(cur, "%2hhx", &fake_ims[i]);
        cur += 2;
    }
    return success;
}

bool cb_key_type(const int option, const char * optarg,
                 struct optionx * optx) {
    bool success = true;
    success = get_num(optarg, (optx->long_names)[0], &key_type);
    return success;
}

int main(int argc, char **argv) {
    bool success = true;
    s2config_descriptor *cfg;
    struct argparse * parse_tbl = NULL;

    /* Parse the command line arguments */
    parse_tbl = new_argparse(parse_table, argv[0], NULL, epilog, NULL,
                             NULL);
    if (parse_tbl) {
        success =  parse_args(argc, argv, all_args, parse_tbl);
    } else {
        success = false;
    }

    if (!success) {
        return -1;
    }

    cfg = new_s2config();

    if (key_name != NULL && key_file_name != NULL) {
        s2config_add_public_key(cfg, key_file_name, key_type, key_name);
    }

    s2config_set_fake_ara_vidpid(cfg, use_fake_vidpid, fake_vid, fake_pid);

    s2config_set_fake_ims(cfg, fake_ims, sizeof(fake_ims));

    print_s2config(cfg);
    save_s2config_to_file(output_filename, cfg);
    free_s2config(cfg);

    parse_tbl = free_argparse(parse_tbl);
    return 0;
}
