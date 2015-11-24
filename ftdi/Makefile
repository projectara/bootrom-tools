#------------------------------------------------------------------------------
# Copyright (c) 2014-2015 Google Inc.
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
#------------------------------------------------------------------------------

# Regarding libMPSSE:
# Download the source from the FTDI site
# (http://www.ftdichip.com/Support/SoftwareExamples/MPSSE/LibMPSSE-SPI.htm) -
# follow the link at the bottom of the text "The source code for the
# LibMPSSE-SPI library is available for download here", and and unzip the
# dowloaded file to your home directory. Once unzipped,
# cd ~/LibMPSSE-SPI_source/LibMPSSE-SPI/LibMPSSE/Build/Linux and type "make"

binaries=haps_test spirom_write
INC_FTD2XX=$(HOME)/LibMPSSE-SPI_source/LibMPSSE-SPI/Release/include/linux
INC_MPSSE=$(HOME)/LibMPSSE-SPI_source/LibMPSSE-SPI/Release/include
LIBMPSSE=~/LibMPSSE-SPI_source/LibMPSSE-SPI/LibMPSSE/Build/Linux/libMPSSE.a
CFLAGS=-g -I. -I$(INC_MPSSE) -I$(INC_FTD2XX)

all: $(binaries)

#spirom_write: spirom_write.o spirom_common.o LibMPSSE-SPI_source/LibMPSSE-SPI/LibMPSSE/Build/Linux/libMPSSE.a
#	gcc -g -I. $^ LibMPSSE-SPI_source/LibMPSSE-SPI/LibMPSSE/Build/Linux/libMPSSE.a -o $@  -ldl

spirom_write: spirom_write.o spirom_common.o
	gcc $(CFLAGS) $^ $(LIBMPSSE) -o $@ -ldl

haps_test: haps_test.o gpio.o common.o jlink_script.o uart.o reset.o settings.h
	gcc $(CFLAGS) $^ -o $@ -ldl -lftd2xx


.PHONY: all clean

clean:
	rm -f $(binaries) *.o



