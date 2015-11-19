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
# ...bootorm_tools/src/display-ffff/Makefile

include $(TOPDIR)/Makefile.inc

ODIR         = obj
COMMONOBJDIR = $(TOPDIR)/obj
BINDIR       = $(TOPDIR)/bin
IDIR         = $(TOPDIR)/include
LIBDIR       = $(TOPDIR)/libs

EXE_NAME = display-ffff
EXE      = $(BINDIR)/$(EXE_NAME)

INC_DIRS := -I. -I$(IDIR)
EXTRA_LIBS := -lelf

_DEPS = \
  util.h \
  parse_support.h \
  ffff.h \
  ffff_common.h \
  ffff_in.h \
  ffff_out.h \
  ffff_map.h \
  ffff_print.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = $(ODIR)/display-ffff.o

_LIBS = -lffff -ltftf -lcommon
_LIBDEPS = libffff.a libcommon.a libtftf.a
LIBDEPS = $(patsubst %,$(LIBDIR)/%,$(_LIBDEPS))

INC_DIRS := -I. -I$(IDIR)
SFILES   := c
OFILES   := o
CC       := gcc
LD       := gcc
CFLAGS   := -c -Wall
EXE      := $(BINDIR)/$(EXE_NAME)

.PHONY: all clean exe

all: $(EXE)

$(EXE): $(OBJ) $(LIBDEPS)
	mkdir -p $(ODIR) $(BINDIR)
	@ echo Compiling exe: $<
	$(CC) $^ $(EXTRA_LIBS) -L$(LIBDIR) $(_LIBS) -lelf -o $@

$(ODIR)/display-ffff.o: display-ffff.c $(DEPS)
	mkdir -p $(ODIR)
	@ echo Compiling $<
	$(CC) $(CFLAGS) $(INC_DIRS) $< -o $@

clean:
	rm -f $(OBJ) $(EXE)


