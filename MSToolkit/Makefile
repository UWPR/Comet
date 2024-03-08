# -- Author's Note ------------------------------------------------------------
#
# Well, it's not the worst Makefile I've ever written...
#
#
.DEFAULT_GOAL := all

C = gcc
CC = gcc
CXX = g++
CFLAGS = -O3 -static -I. -I./include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -DHAVE_EXPAT_CONFIG_H
SFLAGS = -O3 -shared -fPIC -g -I. -I./include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -DHAVE_EXPAT_CONFIG_H
LITEFLAGS = -D_NOSQLITE

UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
SFLAGS += -Wl,-z,norelro
endif


SOVER = 83
RELVER = $(SOVER).0.1

SRC_DIR  := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_SRC = $(SRC_DIR)/src
BUILD_DIR = $(SRC_DIR)/obj
BUILD_LIC = $(SRC_DIR)/lic

# Make build/staging directories. Should be used as a "order-only" prerequisite
# (prefixed with a "|" ) on any build rules.
$(BUILD_DIR)/ :
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_LIC)
	
	

# -- Global build rules -------------------------------------------------------
#
# Rules for all packages of the MSToolkit.
#

.PHONY: all clean realclean
all       : expat zlib mstoolkit mzparser lib
clean     : expat-clean zlib-clean mstoolkit-clean mzparser-clean mzimltools-clean lib-clean
realclean : expat-realclean zlib-realclean mstoolkit-realclean mzparser-realclean mzimltools-realclean lib-realclean rc

# -- Expat XML Parser ---------------------------------------------------------
#
# Expat is an XML parser library written in C. It is a stream-oriented parser in
# which an application registers handlers for things the parser might find in
# the XML document (like start tags).  Released under the MIT license.
#
# http://expat.sourceforge.net/
#
EXPAT_VER := expat-2.2.9
EXPAT_SRC := $(BUILD_SRC)/$(EXPAT_VER)
EXPAT_DST := include/expat_config.h $(BUILD_DIR)/libexpat.a
EXPAT_LIC := $(BUILD_LIC)/$(EXPAT_VER)/COPYING

.PHONY : expat expat-clean expat-realclean

expat : $(EXPAT_DST) $(EXPAT_LIC)

$(EXPAT_DST) : | $(BUILD_DIR)/ $(EXPAT_SRC)/
	cd $(BUILD_SRC); tar -xzf $(EXPAT_VER).tar.gz
	cd $(EXPAT_SRC); ./configure --prefix=$(BUILD_DIR) \
	   --enable-shared=yes --enable-static=yes \
	   --without-xmlwf --without-examples --without-tests \
	   --with-aix-soname=both \
	   --includedir $(SRC_DIR)/include --libdir $(BUILD_DIR) 
	make -C $(EXPAT_SRC) install

$(EXPAT_LIC) : $(EXPAT_SRC)/$(notdir $(EXPAT_LIC))| $(MKDIR)
	mkdir -p $(BUILD_LIC)/$(EXPAT_VER)
	cp $^ $@

expat-clean :
ifneq (,$(wildcard $(EXPAT_SRC)/Makefile))
	make -C $(EXPAT_SRC) clean
endif

expat-realclean : expat-clean
	rm -rf $(EXPAT_LIC) $(EXPAT_DST) $(BUILD_DIR)/libexpat.la $(BUILD_DIR)/libexpat.so* $(BUILD_DIR)/bin $(BUILD_DIR)/man



# -- zlib compression library -------------------------------------------------
#
# A Massively Spiffy Yet Delicately Unobtrusive Compression Library
# (Also Free, Not to Mention Unencumbered by Patents)
# (Not Related to the Linux zlibc Compressing File-I/O Library)
#
# http://zlib.net/
#
ZLIB_VER := zlib-1.2.11
ZLIB_SRC := $(BUILD_SRC)/$(ZLIB_VER)
ZLIB_SRCB := $(wildcard $(ZLIB_SRC)/*.c)
ZLIB_DST := $(BUILD_DIR)/libz.a
ZLIB_LIC := $(BUILD_LIC)/$(ZLIB_VER)/README

.PHONY : zlib zlib-clean zlib-realclean

zlib : $(ZLIB_DST) $(ZLIB_LIC)

$(ZLIB_DST) : | $(BUILD_DIR)/ $(ZLIB_SRC)/
	cd $(BUILD_SRC); unzip -o zlib1211.zip

ifeq ($(OS),Windows_NT)
	cd $(ZLIB_SRC); make -f win32/Makefile.gcc; cp libz.a $(BUILD_DIR)
else
	cd $(ZLIB_SRC); ./configure --prefix=$(BUILD_DIR) --includedir $(SRC_DIR)/include --libdir $(BUILD_DIR)
	make -C $(ZLIB_SRC) install
endif

$(ZLIB_LIC) : $(ZLIB_SRC)/$(notdir $(ZLIB_LIC))| $(MKDIR)
	mkdir -p $(BUILD_LIC)/$(ZLIB_VER)
	cp $^ $@

zlib-clean :
ifneq (,$(wildcard $(ZLIB_SRC)/Makefile))
	make -C $(ZLIB_SRC) distclean
endif

zlib-realclean : zlib-clean
	rm -rf $(ZLIB_LIC) $(ZLIB_DST) $(BUILD_DIR)/libz.so*
	rm -f include/zconf.h include/expat.h include/expat_external.h include/zlib.h
	

# -- MSToolkit  ---------------------------------------------------------------
#
# The core MSToolkit files, but skip RAWReader on Linux systems
#
#
MSTOOLKIT_SRCDIR = $(BUILD_SRC)/MSToolkit/
MSTOOLKIT_DSTDIR = $(BUILD_DIR)/
MSTOOLKIT_SRC = $(filter-out $(BUILD_SRC)/MSToolkit/RAWReader.cpp, $(wildcard $(BUILD_SRC)/MSToolkit/*.cpp))
MSTOOLKIT_DSTLITE = $(patsubst ${MSTOOLKIT_SRCDIR}%.cpp, ${MSTOOLKIT_DSTDIR}%_lite.o, $(MSTOOLKIT_SRC))

.PHONY : mstoolkit

mstoolkit : $(MSTOOLKIT_DSTLITE)
	ar rcs $(BUILD_DIR)/libmstlite.a $(MSTOOLKIT_DSTLITE)

$(MSTOOLKIT_DSTLITE) : $(MSTOOLKIT_DSTDIR)%_lite.o : $(MSTOOLKIT_SRCDIR)%.cpp
	$(CXX) $(CFLAGS) $(LITEFLAGS) $< -c -o $@
	
mstoolkit-clean :
	rm -rf $(MSTOOLKIT_DSTLITE)

mstoolkit-realclean : mstoolkit-clean
	rm -rf $(MSTOOLKIT_DSTLITE) $(BUILD_DIR)/libmstlite.a
	

# -- mzParser  ---------------------------------------------------------------
#
# for reading the many TPP formats, with its own interface (and a RAMP interface)
#
#
MZPARSER_SRCDIR = $(BUILD_SRC)/mzParser/
MZPARSER_DSTDIR = $(BUILD_DIR)/
MZPARSER_SRC = $(filter-out $(BUILD_SRC)/mzParser/mzMLReader.cpp, $(wildcard $(BUILD_SRC)/mzParser/*.cpp))
MZPARSER_DST = $(patsubst ${MZPARSER_SRCDIR}%.cpp, ${MZPARSER_DSTDIR}%.o, $(MZPARSER_SRC))

.PHONY : mzparser

mzparser : $(MZPARSER_DST)
	ar rcs $(BUILD_DIR)/libmzparser.a $(MZPARSER_DST)
	
$(MZPARSER_DST) : | $(BUILD_DIR)/
$(MZPARSER_DST) : $(MZPARSER_DSTDIR)%.o : $(MZPARSER_SRCDIR)%.cpp
	$(CXX) $(CFLAGS) $< -c -o $@
	
mzparser-clean :
	rm -rf $(MZPARSER_DST)

mzparser-realclean : mzparser-clean
	rm -rf $(MZPARSER_DST) $(BUILD_DIR)/libmzparser.a
	

# -- mzIMLTools  ---------------------------------------------------------------
#
# mzIdentMLTools is a c++ style interface for reading and writing mzID files
#
#
MZIMLTOOLS_SRCDIR = $(BUILD_SRC)/mzIMLTools/
MZIMLTOOLS_DSTDIR = $(BUILD_DIR)/
MZIMLTOOLS_SRC = $(wildcard $(BUILD_SRC)/mzIMLTools/*.cpp)
MZIMLTOOLS_DST = $(patsubst ${MZIMLTOOLS_SRCDIR}%.cpp, ${MZIMLTOOLS_DSTDIR}%.o, $(MZIMLTOOLS_SRC))

.PHONY : mzimltools

mzimltools : $(MZIMLTOOLS_DST)
	ar rcs $(BUILD_DIR)/libmzimltools.a $(MZIMLTOOLS_DST)

$(MZIMLTOOLS_DST) : | $(BUILD_DIR)/
$(MZIMLTOOLS_DST) : $(MZIMLTOOLS_DSTDIR)%.o : $(MZIMLTOOLS_SRCDIR)%.cpp
	$(CXX) $(CFLAGS) $< -c -o $@
	
mzimltools-clean :
	rm -rf $(MZIMLTOOLS_DST)

mzimltools-realclean : mzimltools-clean
	rm -rf $(MZIMLTOOLS_DST) $(BUILD_DIR)/libmzimltools.a
	

# -- Libraries  ----------------------------------------------------------
#
# The static libmstoolkit.a and shared libmstoolkit.so (I think)
# not sure how to link expat into the shared library.
#
#
.PHONY : lib

lib : expat zlib mstoolkit mzparser mzimltools
ifneq ($(UNAME_S),Darwin)
	ar -M <mstoolkitlite.mri
else
	ar -qc libmstoolkitlite.a obj/*.o src/expat-2.2.9/lib/*.o src/zlib-1.2.11/*.o
endif


lib-clean :
	rm -rf libmstoolkitlite.a

lib-realclean : lib-clean
	rm -rf libmstoolkitlite.a
	
	
# -- cleanup  ----------------------------------------------------------
#
#
.PHONY : rc

rc:
	rm -rf $(BUILD_DIR) $(BUILD_LIC)
