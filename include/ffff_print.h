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
 * @brief: This file contains the includes for displaying an FFFF file
 *
 */

#ifndef _COMMON_FFFF_DISPLAY_H
#define _COMMON_FFFF_DISPLAY_H


/**
 * @brief Convert an FFFF element type into a human-readable string
 *
 * @param type the FFFF element type
 *
 * @returns A string
 */
const char * ffff_element_type_name(const uint8_t type);


/**
 * @brief Print out an FFFF blob Typically called from "create_ffff".
 *
 * @param romimage Pointer to the FFFF to display
 * @param title_string Optional title string
 * @param indent Optional prefix string
 *
 * @returns Nothing
 */
void print_ffff(const struct ffff *romimage,
                const char * title_string,
                const char * indent);


/**
 * @brief Print out an FFFF file
 *
 * Prints out an in-memory FFFF blob, associated with a file. Typically
 * called from "create_ffff".
 *
 * @param romimage Pointer to the FFFF to display
 * @param filename Name of the FFFF file
 *
 * @returns Nothing
 */
void print_ffff_file(const struct ffff *romimage, const char * filename);

#endif /* _COMMON_FFFF_DISPLAY_H */

