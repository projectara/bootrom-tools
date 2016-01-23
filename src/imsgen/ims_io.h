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
 * @brief: This file describes the I/O subsystem for IMS testing
 *
 */

#ifndef _IMS_IO_H
#define _IMS_IO_H


/**
 * @brief Determine how many IMS values are in an IMS file
 *
 * @param ims_filename The name of the IMS input file
 *
 * @returns If successful, the number (0-n) of IMS values in the file.
 *          Otherwise, a negative number.
 */
int num_ims_in_file(const char * ims_filename);


/**
 * @brief Get an IMS value from a string or the Nth entry in an IMS file
  *
 * Note that if ims_binascii is non-null, ims_filename and ims_index are
 * ignored. If ims_value is null, we use ims_filename and ims_index.
 *
 * @param ims_value (optional) The binascii string representation of the IMS
 * @param ims_filename (optional) The name of the IMS input file
 * @param ims_index (optional) Which IMS value to use from ims_filename
 * @param ims Where to store the IMS
 *
 * @returns 0 if successful, errno or -1 otherwise.
 */
int get_ims(const char * ims_binascii,
            const char * ims_filename,
            const int    ims_index,
            uint8_t      ims[IMS_SIZE]);


/**
 * @brief Write a blob to a named file
 *
 * @param fname The pathanme of the file to create
 * @param buf Points to the buffer to write.
 * @param len The length in bytes of the buffer.
 *
 * @returns 0 if successful, errno or -1 otherwise.
 */
int write_blob(const char * fname, const uint8_t * buf, const size_t len);


/**
 * @brief Write an array of octets to a named file
 *
 * @param fname The pathanme of the file to write
 * @param octet Points to an array of octets to write.
 * @param num_octets The number of octets in the octets array.
 *
 * @returns 0 if successful, errno or -1 otherwise.
 */
int write_octets(const char * fname, mcl_octet * octet, int num_octets);


/**
 * @brief Read a blob from a named file
 *
 * @param fname The pathanme of the file to read
 * @param octet Points to an array of octets to read.
 * @param num_octets The number of octets in the octets array.
 *
 * @returns 0 if successful, errno or -1 otherwise.
 */
int read_octets(const char * fname, mcl_octet * octet, int num_octets);


/**
 * @brief Read a file into an octet
 *
 * Allocates a buffer for an octet and reads a file into it.
 *
 * @param ims_filename The name of the IMS input file
 * @param octet The octet to load
 *
 * @returns If successful, the number (0-n) of IMS values in the file.
 *          Otherwise, a negative number.
 */
int read_file_into_octet(const char * filename, mcl_octet *octet);

#endif /* !_IMS_IO_H */
