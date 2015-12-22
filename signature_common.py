#! /usr/bin/env python

#
# Copyright (c) 2015 Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

## Code common to signature-processing files

from __future__ import print_function
import os


# Recognized algorithm types (--algorithm)
TFTF_SIGNATURE_TYPE_UNKNOWN = 0x00
TFTF_SIGNATURE_ALGORITHM_RSA_2048_SHA_256 = 0x01
TFTF_SIGNATURE_ALGORITHMS = \
    {"rsa2048-sha256": TFTF_SIGNATURE_ALGORITHM_RSA_2048_SHA_256}
TFTF_SIGNATURE_ALGORITHM_NAMES = \
    {TFTF_SIGNATURE_ALGORITHM_RSA_2048_SHA_256: "rsa2048-sha256"}

# Recognized key types (--type)
KEY_TYPE_UNKNOWN = 0
KEY_TYPE_S2FSK = 1
KEY_TYPE_S3FSK = 1
KEY_TYPES = {"s2fsk": KEY_TYPE_S2FSK,
             "s3fsk": KEY_TYPE_S3FSK}
KEY_NAMES = {KEY_TYPE_S2FSK: "s2fsk"}

# Recognized formats (--format)
FORMAT_TYPE_UNKNOWN = 0
FORMAT_TYPE_STANDARD = 1
FORMAT_TYPE_ES3 = 2
FORMAT_TYPES = {"standard": FORMAT_TYPE_STANDARD,
                "es3": FORMAT_TYPE_ES3}
FORMAT_NAMES = {FORMAT_TYPE_STANDARD: "standard",
                FORMAT_TYPE_ES3: "es3"}

# Common command-line arguments for signing apps
SIGNATURE_COMMON_ARGUMENTS = [
#    (["--type"], {"required": True,
#                  "help": "The type of the key file (e.g., s2fsk)"}),
#    (["--format"], {"required": True,
#                    "help": "The naming format for keys (standard | es3)"})
    (["--domain"], {"required": True,
                    "help": "The key domain - the right-hand part of the "
                           "validation key name"}),

    (["--id"], {"help": "The ID of the key (instead of deriving it "
                        "from the key filename)"}),

    (["--algorithm"], {"required": True,
                       "help": "The cryptographic signature algorithm used "
                               "in the PEM file (e.g., rsa2048-sha256)"})]


def rchop(thestring, ending):
    # Remove trailing substing if present
    if thestring.endswith(ending):
        return thestring[:-len(ending)]
    else:
        return thestring


def get_key_filename(filename, private):
    """ Add in any missing extension to the filename

    Check for the specified file, and if that fails, try appending the
    extensions.

    Returns the final filename string if found, None if not found
    """
    if private:
        names = (filename, filename + "private.pem")
    else:
        names = (filename, filename + "public.pem")
    for name in names:
        if os.path.isfile(name):
            return name
    # Can't find the file in any of its variations
    return None


def get_key_id(key_id, key_filename):
    """ Derive the key ID from the key's filename if not user-specified """
    if not key_id:
        # Strip any leading path, and any ".pem", ".private.pem" or
        # ".public.pem" extensions from the filename
        key_id = os.path.basename(key_filename)
        key_id = rchop(key_id, ".private.pem")
        key_id = rchop(key_id, ".public.pem")
        key_id = rchop(key_id, ".pem")
    return key_id


def format_key_name(key_id, key_domain):
    """ Assemble a key name from ID & domain """
    key_name = "{0:s}@{1:s}".format(key_id, key_domain)
    return key_name


def get_signature_algorithm(algorithm_type_string):
    """convert a string into a key_type (TFTF_SIGNATURE_TYPE_xxx)

    returns a numeric key_type, or raises an exception if invalid
    """
    try:
        return TFTF_SIGNATURE_ALGORITHMS[algorithm_type_string]
    except:
        raise ValueError("Unknown algorithm type: '{0:s}'".
                         format(algorithm_type_string))


def get_signature_algorithm_name(algorithm):
    """ Convert a algorithm_type (TFTF_SIGNATURE_TYPE_xxx) into a string

    returns a key name, or raises an exception if invalid
    """
    try:
        return TFTF_SIGNATURE_ALGORITHM_NAMES[algorithm]
    except:
        raise ValueError("Unknown algorithm type: '{0:d}'".format(algorithm))
