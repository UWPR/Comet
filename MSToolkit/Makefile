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

# -- Need to fix HDF5
#ifdef HDF5
#CFLAGS = -O3 -std=c++11 -I. -I./include -I$(HDF5_DIR)/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -DHAVE_EXPAT_CONFIG_H $(HDF5)
#SFLAGS = -O3 -std=c++11 -shared -fPIC -g -I. -I./include -I$(HDF5_DIR)/include  -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -DHAVE_EXPAT_CONFIG_H $(HDF5)
#else
#CFLAGS = -O3 -std=c++11
#endif

SOVER = 2
RELVER = $(SOVER).0.0

SRC_DIR  := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
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
all       : expat zlib mstoolkit mzparser lib libextern MSSingleScan
mst       : mstoolkit mzparser lib MSSingleScan
extern	  : expat zlib libextern
clean     : expat-clean zlib-clean mstoolkit-clean mzparser-clean lib-clean libextern-clean MSSingleScan-clean
realclean : expat-realclean zlib-realclean mstoolkit-realclean mzparser-realclean lib-realclean libextern-realclean MSSingleScan-realclean rc

# -- Expat XML Parser ---------------------------------------------------------
#
# Expat is an XML parser library written in C. It is a stream-oriented parser in
# which an application registers handlers for things the parser might find in
# the XML document (like start tags).  Released under the MIT license.
#
# http://expat.sourceforge.net/
#
EXPAT_VER := expat-2.2.9
EXPAT_SRC := $(BUILD_EXT)/$(EXPAT_VER)
EXPAT_DST := include/extern/expat_config.h $(BUILD_DIR)/libexpat.a
EXPAT_DSO := $(BUILD_EXT)/$(EXPAT_VER)/lib/.libs/xmlparse.o $(BUILD_EXT)/$(EXPAT_VER)/lib/.libs/xmlrole.o $(BUILD_EXT)/$(EXPAT_VER)/lib/.libs/xmltok.o
EXPAT_LIC := $(BUILD_LIC)/$(EXPAT_VER)/COPYING

.PHONY : expat expat-clean expat-realclean

expat : $(EXPAT_DST) $(EXPAT_LIC)

$(EXPAT_DST) : | $(BUILD_DIR)/ $(EXPAT_SRC)/
	cd $(BUILD_EXT); tar -xzf $(EXPAT_VER).tar.gz
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
ZLIB_SRC := $(BUILD_EXT)/$(ZLIB_VER)
ZLIB_SRCB := $(wildcard $(ZLIB_SRC)/*.c)
ZLIB_DST := $(BUILD_DIR)/libz.a
ZLIB_LIC := $(BUILD_LIC)/$(ZLIB_VER)/README
ZLIB_DSO := $(patsubst ${ZLIB_SRC}/%.c, ${ZLIB_SRC}/%.lo, $(ZLIB_SRCB))

.PHONY : zlib zlib-clean zlib-realclean

zlib : $(ZLIB_DST) $(ZLIB_LIC)

$(ZLIB_DST) : | $(BUILD_DIR)/ $(ZLIB_SRC)/
	cd $(BUILD_EXT); unzip -o zlib1211.zip
	cd $(ZLIB_SRC); ./configure --prefix=$(BUILD_DIR) --includedir $(SRC_DIR)/include --libdir $(BUILD_DIR) 
	make -C $(ZLIB_SRC) install

$(ZLIB_LIC) : $(ZLIB_SRC)/$(notdir $(ZLIB_LIC))| $(MKDIR)
	mkdir -p $(BUILD_LIC)/$(ZLIB_VER)
	cp $^ $@

zlib-clean :
ifneq (,$(wildcard $(ZLIB_SRC)/Makefile))
	make -C $(ZLIB_SRC) clean
endif

zlib-realclean : zlib-clean
	rm -rf $(ZLIB_LIC) $(ZLIB_DST) $(BUILD_DIR)/libz.so*
	

# -- MSToolkit  ---------------------------------------------------------------
#
# The core MSToolkit files, but skip RAWReader on Linux systems
#
#
MSTOOLKIT_SRCDIR = $(BUILD_SRC)/MSToolkit/
MSTOOLKIT_DSTDIR = $(BUILD_DIR)/
MSTOOLKIT_SRC = $(filter-out $(BUILD_SRC)/MSToolkit/RAWReader.cpp, $(wildcard $(BUILD_SRC)/MSToolkit/*.cpp))
MSTOOLKIT_DST = $(patsubst ${MSTOOLKIT_SRCDIR}%.cpp, ${MSTOOLKIT_DSTDIR}%.o, $(MSTOOLKIT_SRC))
MSTOOLKIT_DSO = $(patsubst ${MSTOOLKIT_SRCDIR}%.cpp, ${MSTOOLKIT_DSTDIR}%.lo, $(MSTOOLKIT_SRC))

.PHONY : mstoolkit

mstoolkit : $(MSTOOLKIT_DST) $(MSTOOLKIT_DSO)
	ar rcs $(BUILD_DIR)/libmst.a $(MSTOOLKIT_DST)

$(MSTOOLKIT_DST) : | $(BUILD_DIR)/
$(MSTOOLKIT_DST) : $(MSTOOLKIT_DSTDIR)%.o : $(MSTOOLKIT_SRCDIR)%.cpp
	$(CC) $(CFLAGS) -static $(INCLUDE) $(DFLAGS) $< -c -o $@

$(MSTOOLKIT_DSO) : $(MSTOOLKIT_DSTDIR)%.lo : $(MSTOOLKIT_SRCDIR)%.cpp
	$(CC) $(CFLAGS) $(SLFLAGS) $(INCLUDE) $(DFLAGS) $< -c -o $@
	
mstoolkit-clean :
	rm -rf $(MSTOOLKIT_DST) $(MSTOOLKIT_DSO)

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

mzparser : $(MZPARSER_DST) $(MZPARSER_DSO)
	ar rcs $(BUILD_DIR)/libmzparser.a $(MZPARSER_DST)
	$(CC) $(CFLAGS) $(SLFLAGS) $(INCLUDE) $(DFLAGS) -o $(BUILD_DIR)/libmzparser.so.$(RELVER) -Wl,-z,relro -Wl,-soname,libmzparser.so.$(SOVER) $(MZPARSER_DSO)
	ln -sf $(BUILD_DIR)/libmzparser.so.$(RELVER) $(BUILD_DIR)/libmzparser.so.$(SOVER)
	ln -sf $(BUILD_DIR)/libmzparser.so.$(SOVER) $(BUILD_DIR)/libmzparser.so
	
$(MZPARSER_DST) : | $(BUILD_DIR)/
$(MZPARSER_DST) : $(MZPARSER_DSTDIR)%.o : $(MZPARSER_SRCDIR)%.cpp
	$(CC) $(CFLAGS) -static $(INCLUDE) $(DFLAGS) $< -c -o $@
	
$(MZPARSER_DSO) : $(MZPARSER_DSTDIR)%.lo : $(MZPARSER_SRCDIR)%.cpp
	$(CC) $(CFLAGS) $(SLFLAGS) $(INCLUDE) $(DFLAGS)  $< -c -o $@
	
mzparser-clean :
	rm -rf $(MZPARSER_DST) $(MZPARSER_DSO)

mzparser-realclean : mzparser-clean
	rm -rf $(MZPARSER_DST) $(MZPARSER_DSO) $(BUILD_DIR)/libmzparser.a $(BUILD_DIR)/libmzparser.so*
	

# -- Libraries  ----------------------------------------------------------
#
# The static libmstoolkit.a and shared libmstoolkit.so 
#
#
.PHONY : lib

lib : mstoolkit mzparser
	$(file >mstoolkit.mri,create libmstoolkit.a)
	$(file >>mstoolkit.mri,addlib obj/libmst.a)
	$(file >>mstoolkit.mri,addlib obj/libmzparser.a)
ifdef HDF5
	$(file >>mstoolkit.mri,addlib $(HDF5_DIR)/lib/libhdf5.a)
	$(file >>mstoolkit.mri,addlib $(HDF5_DIR)/lib/libhdf5_cpp.a)
endif
	$(file >>mstoolkit.mri,save)
	$(file >>mstoolkit.mri,end)
	ar -M <mstoolkit.mri
	$(CC) $(CFLAGS) $(SLFLAGS) $(INCLUDE) $(DFLAGS) -o libmstoolkit.so.$(RELVER) -Wl,-z,relro -Wl,-soname,libmstoolkit.so.$(SOVER) $(MSTOOLKIT_DSO) $(MZPARSER_DSO)
	ln -sf libmstoolkit.so.$(RELVER) libmstoolkit.so.$(SOVER)
	ln -sf libmstoolkit.so.$(SOVER) libmstoolkit.so

lib-clean :
	rm -rf libmstoolkit.a libmstoolkit.so*

lib-realclean : lib-clean
	rm -rf libmstoolkit.a libmstoolkit.so*


# -- External Libraries  ----------------------------------------------------------
#
# The static libmstoolkitextern.a and shared libmstoolkitextern.so (I think)
#
#
.PHONY : libextern

libextern : expat zlib
	$(file >mstoolkitextern.mri,create libmstoolkitextern.a)
	$(file >>mstoolkitextern.mri,addlib obj/libexpat.a)
	$(file >>mstoolkitextern.mri,addlib obj/libz.a)
ifdef HDF5
	$(file >>mstoolkitextern.mri,addlib $(HDF5_DIR)/lib/libhdf5.a)
	$(file >>mstoolkitextern.mri,addlib $(HDF5_DIR)/lib/libhdf5_cpp.a)
endif
	$(file >>mstoolkitextern.mri,save)
	$(file >>mstoolkitextern.mri,end)
	ar -M <mstoolkitextern.mri
	$(CC) $(CFLAGS) $(SLFLAGS) $(INCLUDE) $(DFLAGS) -o libmstoolkitextern.so.$(RELVER) -Wl,-z,relro -Wl,-soname,libmstoolkitextern.so.$(SOVER) $(ZLIB_DSO) $(EXPAT_DSO) 
	ln -sf libmstoolkitextern.so.$(RELVER) libmstoolkitextern.so.$(SOVER)
	ln -sf libmstoolkitextern.so.$(SOVER) libmstoolkitextern.so

libextern-clean :
	rm -rf libmstoolkitextern.a libmstoolkitextern.so*

libextern-realclean : libextern-clean
	rm -rf libmstoolkitextern.a libmstoolkitextern.so*
	
# -- MSSingleScan  ----------------------------------------------------------
#
# Simple example program that utilizes the MSToolkit
#
#
.PHONY : MSSingleScan

MSSingleScan : lib
ifdef HDF5
	$(CC) $(CFLAGS) -static $(INCLUDE) $(DFLAGS) $(SRC_DIR)/MSSingleScanSrc/MSSingleScan.cpp -L. -l:libmstoolkit.a -l:libmstoolkitextern.a -ldl -o MSSingleScan
else
	$(CC) $(CFLAGS) -static $(INCLUDE) $(INCLUDE_EXT) $(DFLAGS) $(SRC_DIR)/MSSingleScanSrc/MSSingleScan.cpp -L. -lmstoolkit -lmstoolkitextern -o MSSingleScan
	$(CC) $(CFLAGS) $(INCLUDE) $(INCLUDE_EXT) $(DFLAGS) $(SRC_DIR)/MSSingleScanSrc/MSSingleScan.cpp -L. -lmstoolkit -lmstoolkitextern -o MSSingleScanSL
endif

MSSingleScan-clean :
	rm -rf MSSingleScan MSSingleScanSL

MSSingleScan-realclean : MSSingleScan-clean
	rm -rf MSSingleScan MSSingleScanSL

	
# -- cleanup  ----------------------------------------------------------
#
#
.PHONY : rc

rc:
	rm -rf $(BUILD_DIR) $(BUILD_LIC)
