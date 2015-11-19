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
# ...bootrom-tools/src/commom/Makefile

include $(TOPDIR)/Makefile.inc

IDIR = $(TOPDIR)/include
#ODIR = obj
ODIR = $(TOPDIR)/obj
LIBDIR = $(TOPDIR)/libs

_DEPS = \
  parse_support.h \
  util.h \
  tftf_common.h \
  tftf_in.h \
  tftf_out.h \
  tftf_map.h \
  tftf_print.h \
  ffff_common.h \
  ffff_in.h \
  ffff_out.h \
  ffff_map.h \
  ffff_print.h

DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_COMMONOBJ = \
  parse_support.o \
  util.o

_TFTFOBJ = \
  tftf_common.o \
  tftf_in.o \
  tftf_out.o \
  tftf_map.o \
  tftf_print.o

_FFFFOBJ = \
  ffff_common.o \
  ffff_in.o \
  ffff_out.o \
  ffff_map.o \
  ffff_print.o

_OBJ = $(_COMMONOBJ) $(_TFTFOBJ) $(_FFFFOBJ)

COMMONOBJ = $(patsubst %,$(ODIR)/%,$(_COMMONOBJ))
TFTFOBJ = $(patsubst %,$(ODIR)/%,$(_TFTFOBJ))
FFFFOBJ = $(patsubst %,$(ODIR)/%,$(_FFFFOBJ))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_LIBS = libcommon.a libtftf.a libffff.a
LIBS = $(patsubst %,$(LIBDIR)/%,$(_LIBS))

INC_DIRS := -I. -I$(IDIR)

all: $(OBJ) $(LIBS)


$(LIBDIR)/libcommon.a: $(COMMONOBJ)
	mkdir -p $(dir $@)
	ar rcs $@ $^
	ranlib $@

$(LIBDIR)/libtftf.a: $(TFTFOBJ)
	mkdir -p $(dir $@)
	ar rcs $@ $^
	ranlib $@

$(LIBDIR)/libffff.a: $(FFFFOBJ)
	mkdir -p $(dir $@)
	ar rcs $@ $^
	ranlib $@



$(ODIR)/parse_support.o: parse_support.c $(IDIR)/parse_support.h $(IDIR)/ffff.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/util.o: util.c $(IDIR)/util.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/tftf_common.o: tftf_common.c $(IDIR)/tftf_common.h $(IDIR)/tftf.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/tftf_in.o: tftf_in.c $(IDIR)/util.h $(IDIR)/tftf.h $(IDIR)/tftf_in.h $(IDIR)/tftf_common.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/tftf_out.o: tftf_out.c $(IDIR)/util.h $(IDIR)/tftf.h $(IDIR)/tftf_out.h $(IDIR)/tftf_common.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/tftf_map.o: tftf_map.c $(IDIR)/util.h $(IDIR)/tftf.h $(IDIR)/tftf_map.h $(IDIR)/tftf_common.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/tftf_print.o: tftf_print.c $(IDIR)/util.h $(IDIR)/tftf.h $(IDIR)/tftf_print.h $(IDIR)/tftf_common.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/ffff_common.o: ffff_common.c $(IDIR)/ffff_common.h $(IDIR)/ffff.h $(IDIR)/util.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/ffff_in.o: ffff_in.c $(IDIR)/util.h $(IDIR)/ffff.h $(IDIR)/ffff_in.h $(IDIR)/ffff_common.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/ffff_out.o: ffff_out.c $(IDIR)/util.h $(IDIR)/ffff.h $(IDIR)/ffff_out.h $(IDIR)/ffff_common.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/ffff_map.o: ffff_map.c $(IDIR)/util.h $(IDIR)/ffff.h $(IDIR)/ffff_map.h $(IDIR)/ffff_common.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)

$(ODIR)/ffff_print.o: ffff_print.c $(IDIR)/util.h $(IDIR)/ffff.h $(IDIR)/ffff_print.h $(IDIR)/ffff_common.h
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_DIRS)


.PHONY: all clean

clean:
	rm -f $(ODIR)/*.o $(LIBDIR)/*.a *~ core $(INCDIR)/*~


