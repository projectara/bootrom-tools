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
#------------------------------------------------------------------------------
# ...<root>/src/imsgen/Makefile

include $(TOPDIR)/Makefile.inc

# MIRACL library support
MCL_DIR      = $(TOPDIR)/src/vendors/MIRACL/ara
MCL_INCDIR   = $(MCL_DIR)/include
MCL_LIBDIR   = $(MCL_DIR)/bin
MCL_ODIR     = $(MCL_DIR)/bin
#_MCL_LIBS    = -lmclcore -lmclcurveEC
#_MCL_LIBDEPS = libmclcore.a libmclcurveC25519.a
_MCL_LIBS    = -lmclcore -lmclcurveC25519 -lmclcurveC448
_MCL_LIBDEPS = libmclcore.a libmclcurveC25519.a libmclcurveC448.a
MCLLIBDEPS   = $(patsubst %,$(MCL_LIBDIR)/%,$(_MCL_LIBDEPS))
MCL_CFLAGS   = -DC99
MCL_OBJ      = $(MCL_LIBDIR)/mcl_ecdhC25519.o $(MCL_LIBDIR)/mcl_ecdhC448.o
MCL_OBJTEST  = $(MCL_LIBDIR)/mcl_ecdhC25519.o $(MCL_LIBDIR)/mcl_ecdhC448.o $(MCL_LIBDIR)/mcl_rsaRSA2048.o

# Include build configuration
MCL_CONFIG_DIR = $(MCL_DIR)
include $(MCL_CONFIG_DIR)/defconfig
include $(MCL_CONFIG_DIR)/config.mk


EXE_NAME = imsgen
EXE      = $(BINDIR)/$(EXE_NAME)

EXETEST_NAME = imsgen_test
EXETEST      = $(BINDIR)/$(EXETEST_NAME)

EXETEST1_NAME = imsgen_test1
EXETEST1      = $(BINDIR)/$(EXETEST1_NAME)

EXETEST2_NAME = imsgen_test2
EXETEST2      = $(BINDIR)/$(EXETEST2_NAME)

COMMON_NAMES := \
  $(COMMONDIR)/parse_support.c \
  $(COMMONDIR)/util.c

INC_DIRS := -I. $(COMMON_INCDIRS) -I $(MCL_INCDIR)
EXTRA_LIBS := -lcrypto -lsqlite3

_LIBS = -lcommon
_LIBDEPS = libcommon.a
LIBDEPS = $(patsubst %,$(LIBDIR)/%,$(_LIBDEPS))

COMMONTESTOBJ = $(ODIR)/ims_common.o $(ODIR)/ims_io.o $(ODIR)/ims_test_core.o $(ODIR)/crypto.o $(ODIR)/db.o
OBJ      = $(ODIR)/imsgen.o $(ODIR)/ims_common.o $(ODIR)/ims.o $(ODIR)/crypto.o $(ODIR)/db.o
OBJTEST  = $(ODIR)/imsgen_test.o $(ODIR)/ims_test.o $(COMMONTESTOBJ)
OBJTEST1 = $(ODIR)/imsgen_test1.o $(ODIR)/ims_test1.o $(COMMONTESTOBJ)
OBJTEST2 = $(ODIR)/imsgen_test2.o $(ODIR)/ims_test2.o $(COMMONTESTOBJ)

CFLAGS += -DC99 -DMCL_CHUNK=64 -DMCL_FFLEN=8

.PHONY: all clean exe

all: $(EXE) $(EXETEST) $(EXETEST1) $(EXETEST2)

# IMS Generator
$(EXE): $(OBJ) $(LIBDEPS)
	mkdir -p $(ODIR) $(BINDIR)
	@ echo Compiling exe $<
	$(CC) $(CFLAGS) $^ $(MCL_OBJ) $(EXTRA_LIBS) -L$(LIBDIR) $(_LIBS) -L$(MCL_LIBDIR) $(_MCL_LIBS) -o $@

# Bringup IMS test
$(EXETEST): $(OBJTEST) $(LIBDEPS)
	mkdir -p $(ODIR) $(BINDIR)
	@ echo Compiling $(EXETEST_NAME) $<
	$(CC) $(CFLAGS) $^ $(MCL_OBJTEST) $(EXTRA_LIBS) -L$(LIBDIR) $(_LIBS) -L$(MCL_LIBDIR) $(_MCL_LIBS) -o $@

# Official IMS test, part 1 (signs a file with keys from an IMS and saves the signatures)
$(EXETEST1): $(OBJTEST1) $(LIBDEPS)
	mkdir -p $(ODIR) $(BINDIR)
	@ echo Compiling $(EXETEST1_NAME) $<
	$(CC) $(CFLAGS) $^ $(MCL_OBJTEST) $(EXTRA_LIBS) -L$(LIBDIR) $(_LIBS) -L$(MCL_LIBDIR) $(_MCL_LIBS) -o $@

# Official IMS test, part 2 (verifies a file with the keys saved in part 1)
$(EXETEST2): $(OBJTEST2) $(LIBDEPS)
	mkdir -p $(ODIR) $(BINDIR)
	@ echo Compiling $(EXETEST2_NAME) $<
	$(CC) $(CFLAGS) $^ $(MCL_OBJTEST) $(EXTRA_LIBS) -L$(LIBDIR) $(_LIBS) -L$(MCL_LIBDIR) $(_MCL_LIBS) -o $@

-include $(OBJ:.o=.d)

clean:
	rm -f $(OBJ) $(EXE)

