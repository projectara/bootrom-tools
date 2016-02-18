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

_S2CFGOBJ = s2cfgcore.o

_OBJ = $(_COMMONOBJ) $(_TFTFOBJ) $(_FFFFOBJ) $(_S2CFGOBJ)

COMMONOBJ = $(patsubst %,$(ODIR)/%,$(_COMMONOBJ))
TFTFOBJ = $(patsubst %,$(ODIR)/%,$(_TFTFOBJ))
FFFFOBJ = $(patsubst %,$(ODIR)/%,$(_FFFFOBJ))
S2CFGOBJ = $(patsubst %,$(ODIR)/%,$(_S2CFGOBJ))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_LIBS = libcommon.a libtftf.a libffff.a libs2cfg.a
LIBS = $(patsubst %,$(LIBDIR)/%,$(_LIBS))

INC_DIRS := -I. $(COMMON_INCDIRS)

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

$(LIBDIR)/libs2cfg.a: $(S2CFGOBJ)
	mkdir -p $(dir $@)
	ar rcs $@ $^
	ranlib $@

-include $(OBJ:.o=.d)

.PHONY: all clean

clean:
	- rm -rf $(ODIR) $(LIBDIR)/*.a *~ core $(INCDIR)/*~


