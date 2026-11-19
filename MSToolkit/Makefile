# -- Author's Note ------------------------------------------------------------
#
# Well, it's not the worst Makefile I've ever written...
#
#
.DEFAULT_GOAL := all

# -- Uncomment these two variables, and set the correct path, to support HDF5 mass spec formats (.mz5 and .mzMLb)
#HDF5 = -DMZP_HDF
#HDF5_DIR = ../hdf5-1.14.0/hdf5

C = gcc
CC = g++

CFLAGS = -O3 -std=c++11
SLFLAGS = -shared -fPIC -g
INCLUDE = -I./include
INCLUDE_EXT = -I./include/extern
DFLAGS = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC
DFLAGS_EXT = -DHAVE_EXPAT_CONFIG_H
# Automatic header dependencies: each compile writes <object>.d (the .o and the .lo
# of one source get separate files, so -j builds of both never race on one name);
# included at the bottom so a header change recompiles the objects that include it.
# Each object also depends on its own .d file, so an object with no .d (a deleted
# .d, or one built by a Makefile that did not pass -MMD) recompiles instead of being
# kept with no header prerequisites; the empty rule at the bottom "remakes" a
# missing .d by doing nothing.
DEPFLAGS = -MMD -MP -MF $@.d

UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
SFLAGS += -Wl,-z,norelro
endif


# -- Need to fix HDF5
#ifdef HDF5
#CFLAGS = -O3 -std=c++11 -I. -I./include -I$(HDF5_DIR)/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -DHAVE_EXPAT_CONFIG_H $(HDF5)
#SFLAGS = -O3 -std=c++11 -shared -fPIC -g -I. -I./include -I$(HDF5_DIR)/include  -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -DHAVE_EXPAT_CONFIG_H $(HDF5)
#else
#CFLAGS = -O3 -std=c++11
#endif

SOVER = 2
RELVER = $(SOVER).0.0

# Relative to the directory make runs in (the top Makefile runs this one as `cd dir && make`),
# not abspath: the .d files record the object paths as written here, so absolute ones went
# stale when a built tree was copied or renamed and no header change rebuilt anything there.
# ABS_DIR is for the configure steps below, which run from another directory and need absolute prefixes.
SRC_DIR  := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
ABS_DIR  := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_SRC = $(SRC_DIR)/src
BUILD_EXT = $(SRC_DIR)/extern
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

.PHONY: all mst extern clean realclean
all       : expat zlib mstoolkit mzparser lib libextern 
mst       : mstoolkit mzparser lib 
extern	  : expat zlib libextern
clean     : expat-clean zlib-clean mstoolkit-clean mzparser-clean lib-clean libextern-clean 
realclean : expat-realclean zlib-realclean mstoolkit-realclean mzparser-realclean lib-realclean libextern-realclean rc

# -- Expat XML Parser ---------------------------------------------------------
#
# Expat is an XML parser library written in C. It is a stream-oriented parser in
# which an application registers handlers for things the parser might find in
# the XML document (like start tags).  Released under the MIT license.
#
# http://expat.sourceforge.net/
#
EXPAT_VER := expat-2.2.9
EXPAT_ZIP := $(EXPAT_VER).zip
EXPAT_SRC := $(BUILD_EXT)/$(EXPAT_VER)
EXPAT_DST := include/expat_config.h $(BUILD_DIR)/libexpat.a
EXPAT_DSO := $(BUILD_EXT)/$(EXPAT_VER)/lib/.libs/xmlparse.o $(BUILD_EXT)/$(EXPAT_VER)/lib/.libs/xmlrole.o $(BUILD_EXT)/$(EXPAT_VER)/lib/.libs/xmltok.o
EXPAT_STAMP := $(BUILD_DIR)/.expat_installed

.PHONY : expat expat-clean expat-realclean

expat : $(EXPAT_STAMP)

$(EXPAT_SRC)/ :
	cd $(BUILD_EXT); unzip -o -qq $(EXPAT_ZIP)

$(EXPAT_STAMP) : | $(BUILD_DIR)/ $(EXPAT_SRC)/
	cd $(EXPAT_SRC); ./configure --prefix=$(ABS_DIR)/obj \
	   --enable-shared=yes --enable-static=yes \
	   --without-xmlwf --without-examples --without-tests \
	   --with-aix-soname=both \
	   --includedir $(ABS_DIR)/include --libdir $(ABS_DIR)/obj
	make -C $(EXPAT_SRC) install
	touch $(EXPAT_STAMP)

expat-clean :
	rm -rf $(EXPAT_SRC) $(EXPAT_STAMP)

expat-realclean : expat-clean
	rm -rf $(EXPAT_DST) $(EXPAT_STAMP) $(BUILD_DIR)/libexpat.la $(BUILD_DIR)/libexpat.so* $(BUILD_DIR)/bin $(BUILD_DIR)/man $(SRC_DIR)/include/expat*.h



# -- zlib compression library -------------------------------------------------
#
# A Massively Spiffy Yet Delicately Unobtrusive Compression Library
# (Also Free, Not to Mention Unencumbered by Patents)
# (Not Related to the Linux zlibc Compressing File-I/O Library)
#
# http://zlib.net/
#
ZLIB_VER := zlib-1.2.11
ZLIB_SRC := $(BUILD_EXT)/$(ZLIB_VER)
ZLIB_SRCB := $(wildcard $(ZLIB_SRC)/*.c)
ZLIB_DST := $(BUILD_DIR)/libz.a
ZLIB_DSO := $(patsubst ${ZLIB_SRC}/%.c, ${ZLIB_SRC}/%.lo, $(ZLIB_SRCB))

# zlib 1.2.11's zutil.h defines fdopen(fd,mode) as NULL on Apple platforms, which collides with the
# macOS SDK's own fdopen prototype and stops the build. Defining fdopen as itself makes zutil.h's
# "#ifndef fdopen" skip that definition. This replaces patching zutil.h (a CRLF patch file did not apply
# with macOS's BSD patch): it needs no extra tool and leaves the extracted source untouched.
ifeq ($(UNAME_S),Darwin)
ZLIB_ENV := CFLAGS="-O3 -Dfdopen=fdopen"
endif

.PHONY : zlib zlib-clean zlib-realclean

zlib : $(ZLIB_DST)

$(ZLIB_SRC)/ :
	cd $(BUILD_EXT); unzip -o -qq zlib1211.zip

$(ZLIB_DST) : | $(BUILD_DIR)/ $(ZLIB_SRC)/
	cd $(ZLIB_SRC); $(ZLIB_ENV) ./configure --prefix=$(ABS_DIR)/obj --includedir $(ABS_DIR)/include --libdir $(ABS_DIR)/obj
	make -C $(ZLIB_SRC) install

zlib-clean :
	rm -rf $(ZLIB_SRC)

zlib-realclean : zlib-clean
	rm -rf $(ZLIB_DST) $(BUILD_DIR)/libz.so* $(SRC_DIR)/include/zlib.h $(SRC_DIR)/include/zconf.h
	

# -- MSToolkit  ---------------------------------------------------------------
#
# The core MSToolkit files, but skip RAWReader on Linux systems. RAWReader.cpp reads Thermo .raw
# files via RawFileReader .NET through a /clr (C++/CLI) build, which only exists on MSVC/Windows --
# .raw support is Windows-only by decision (see docs/20260618_RawFileReaderMigration.md), not
# because of any Linux-specific limitation in the underlying RawFileReader .NET library itself.
#
MSTOOLKIT_SRCDIR = $(BUILD_SRC)/MSToolkit/
MSTOOLKIT_DSTDIR = $(BUILD_DIR)/
MSTOOLKIT_SRC = $(filter-out $(BUILD_SRC)/MSToolkit/RAWReader.cpp, $(wildcard $(BUILD_SRC)/MSToolkit/*.cpp))
MSTOOLKIT_DST = $(patsubst ${MSTOOLKIT_SRCDIR}%.cpp, ${MSTOOLKIT_DSTDIR}%.o, $(MSTOOLKIT_SRC))
MSTOOLKIT_DSO = $(patsubst ${MSTOOLKIT_SRCDIR}%.cpp, ${MSTOOLKIT_DSTDIR}%.lo, $(MSTOOLKIT_SRC))

.PHONY : mstoolkit

# Each archive below is a file target that depends on what goes into it, so a 'make all'
# with nothing changed rewrites no library: the top-level comet.exe depends on
# libmstoolkit.a / libmstoolkitextern.a and relinks only when one of them really changed.
# (Before 2026-11-09 these were recipes of phony targets and re-ran on every make.)
MSTOOLKIT_LIB = $(BUILD_DIR)/libmst.a

mstoolkit : $(MSTOOLKIT_LIB) $(MSTOOLKIT_DSO)

$(MSTOOLKIT_LIB) : $(MSTOOLKIT_DST)
	rm -f $@
	ar rcs $@ $(MSTOOLKIT_DST)

# The expat/zlib headers these sources include (via mzParser.h) are installed into
# include/ by the expat and zlib targets, so every object waits for both (order-only,
# so a rebuilt library never by itself forces a recompile) -- otherwise 'make -j'
# can start compiling before expat.h exists.
$(MSTOOLKIT_DST) $(MSTOOLKIT_DSO) : | $(BUILD_DIR)/ $(EXPAT_STAMP) $(ZLIB_DST)
$(MSTOOLKIT_DST) : $(MSTOOLKIT_DSTDIR)%.o : $(MSTOOLKIT_SRCDIR)%.cpp $(MSTOOLKIT_DSTDIR)%.o.d
	$(CC) $(CFLAGS) -static $(INCLUDE) $(DFLAGS) $(DEPFLAGS) $< -c -o $@

$(MSTOOLKIT_DSO) : $(MSTOOLKIT_DSTDIR)%.lo : $(MSTOOLKIT_SRCDIR)%.cpp $(MSTOOLKIT_DSTDIR)%.lo.d
	$(CC) $(CFLAGS) $(SLFLAGS) $(INCLUDE) $(DFLAGS) $(DEPFLAGS) $< -c -o $@

mstoolkit-clean :
	rm -rf $(MSTOOLKIT_DST) $(MSTOOLKIT_DSO) $(addsuffix .d, $(MSTOOLKIT_DST) $(MSTOOLKIT_DSO))

mstoolkit-realclean : mstoolkit-clean
	rm -rf $(MSTOOLKIT_DST) $(MSTOOLKIT_DSO) $(BUILD_DIR)/libmst.a 
	

# -- mzParser  ---------------------------------------------------------------
#
# for reading the many TPP formats, with its own interface (and a RAMP interface)
#
#
MZPARSER_SRCDIR = $(BUILD_SRC)/mzParser/
MZPARSER_DSTDIR = $(BUILD_DIR)/
MZPARSER_SRC = $(filter-out $(BUILD_SRC)/mzParser/mzMLReader.cpp, $(wildcard $(BUILD_SRC)/mzParser/*.cpp))
MZPARSER_DST = $(patsubst ${MZPARSER_SRCDIR}%.cpp, ${MZPARSER_DSTDIR}%.o, $(MZPARSER_SRC))
MZPARSER_DSO = $(patsubst ${MZPARSER_SRCDIR}%.cpp, ${MZPARSER_DSTDIR}%.lo, $(MZPARSER_SRC))

.PHONY : mzparser

MZPARSER_LIB = $(BUILD_DIR)/libmzparser.a

mzparser : $(MZPARSER_LIB) $(MZPARSER_DSO)

$(MZPARSER_LIB) : $(MZPARSER_DST)
	rm -f $@
	ar rcs $@ $(MZPARSER_DST)
	
$(MZPARSER_DST) $(MZPARSER_DSO) : | $(BUILD_DIR)/ $(EXPAT_STAMP) $(ZLIB_DST)
$(MZPARSER_DST) : $(MZPARSER_DSTDIR)%.o : $(MZPARSER_SRCDIR)%.cpp $(MZPARSER_DSTDIR)%.o.d
	$(CC) $(CFLAGS) -static $(INCLUDE) $(DFLAGS) $(DEPFLAGS) $< -c -o $@

$(MZPARSER_DSO) : $(MZPARSER_DSTDIR)%.lo : $(MZPARSER_SRCDIR)%.cpp $(MZPARSER_DSTDIR)%.lo.d
	$(CC) $(CFLAGS) $(SLFLAGS) $(INCLUDE) $(DFLAGS) $(DEPFLAGS) $< -c -o $@

mzparser-clean :
	rm -rf $(MZPARSER_DST) $(MZPARSER_DSO) $(addsuffix .d, $(MZPARSER_DST) $(MZPARSER_DSO))

mzparser-realclean : mzparser-clean
	rm -rf $(MZPARSER_DST) $(MZPARSER_DSO) $(BUILD_DIR)/libmzparser.a
	

# -- Libraries  ----------------------------------------------------------
#
# The static libmstoolkit.a and shared libmstoolkit.so 
#
#
.PHONY : lib

lib : mstoolkit mzparser libmstoolkit.a

libmstoolkit.a : $(MSTOOLKIT_LIB) $(MZPARSER_LIB)
	$(file >mstoolkit.mri,create libmstoolkit.a)
	$(file >>mstoolkit.mri,addlib obj/libmst.a)
	$(file >>mstoolkit.mri,addlib obj/libmzparser.a)
ifdef HDF5
	$(file >>mstoolkit.mri,addlib $(HDF5_DIR)/lib/libhdf5.a)
	$(file >>mstoolkit.mri,addlib $(HDF5_DIR)/lib/libhdf5_cpp.a)
endif
	$(file >>mstoolkit.mri,save)
	$(file >>mstoolkit.mri,end)
	
ifneq ($(UNAME_S),Darwin)
	ar -M <mstoolkit.mri
else
	ar -qc libmstoolkit.a obj/*.o
endif


lib-clean :
	rm -rf libmstoolkit.a mstoolkit.mri

lib-realclean : lib-clean
	rm -rf libmstoolkit.a mstoolkit.mri


# -- External Libraries  ----------------------------------------------------------
#
# The static libmstoolkitextern.a and shared libmstoolkitextern.so (I think)
#
#
.PHONY : libextern

libextern : expat zlib libmstoolkitextern.a

libmstoolkitextern.a : $(EXPAT_STAMP) $(ZLIB_DST)
	$(file >mstoolkitextern.mri,create libmstoolkitextern.a)
	$(file >>mstoolkitextern.mri,addlib obj/libexpat.a)
	$(file >>mstoolkitextern.mri,addlib obj/libz.a)
ifdef HDF5
	$(file >>mstoolkitextern.mri,addlib $(HDF5_DIR)/lib/libhdf5.a)
	$(file >>mstoolkitextern.mri,addlib $(HDF5_DIR)/lib/libhdf5_cpp.a)
endif
	$(file >>mstoolkitextern.mri,save)
	$(file >>mstoolkitextern.mri,end)
	
ifneq ($(UNAME_S),Darwin)
	ar -M <mstoolkitextern.mri
else
	ar -qc libmstoolkitextern.a extern/expat-2.2.9/lib/*.o extern/zlib-1.2.11/*.o
endif

libextern-clean :
	rm -rf libmstoolkitextern.a mstoolkitextern.mri

libextern-realclean : libextern-clean
	rm -rf libmstoolkitextern.a mstoolkitextern.mri
	

# -- cleanup  ----------------------------------------------------------
#
#
.PHONY : rc

rc:
	rm -rf $(BUILD_DIR) $(BUILD_LIC) mstoolkitlite.mri

MSTOOLKIT_DEP = $(addsuffix .d, $(MSTOOLKIT_DST) $(MSTOOLKIT_DSO) $(MZPARSER_DST) $(MZPARSER_DSO))
$(MSTOOLKIT_DEP) : ;
.PRECIOUS : $(MSTOOLKIT_DEP)
-include $(MSTOOLKIT_DEP)
