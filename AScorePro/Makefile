.DEFAULT_GOAL := all

CC = g++
CFLAGS = -O3 -std=c++14
SLFLAGS = -shared -fPIC -g
INCLUDE = -I./include
DFLAGS = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC

SRC_DIR  := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_SRC = $(SRC_DIR)
BUILD_DIR = $(SRC_DIR)/obj

ASCOREPRO_SRCDIR = $(BUILD_SRC)/
ASCOREPRO_DSTDIR = $(BUILD_DIR)/
ASCOREPRO_SRC = $(wildcard $(ASCOREPRO_SRCDIR)*.cpp)
ASCOREPRO_DST = $(patsubst ${ASCOREPRO_SRCDIR}%.cpp, ${ASCOREPRO_DSTDIR}%.o, $(ASCOREPRO_SRC))
ASCOREPRO_DSO = $(patsubst ${ASCOREPRO_SRCDIR}%.cpp, ${ASCOREPRO_DSTDIR}%.lo, $(ASCOREPRO_SRC))

.PHONY : all ascorepro clean realclean lib

all       : ascorepro lib
ascorepro : $(ASCOREPRO_DST) $(ASCOREPRO_DSO)
	ar rcs libascorepro.a $(ASCOREPRO_DST)

# Create obj/ before compiling object files
$(BUILD_DIR)/ :
	mkdir -p $(BUILD_DIR)

$(ASCOREPRO_DST) : | $(BUILD_DIR)/
$(ASCOREPRO_DST) : $(ASCOREPRO_DSTDIR)%.o : $(ASCOREPRO_SRCDIR)%.cpp
	$(CC) $(CFLAGS) -static $(INCLUDE) $(DFLAGS) $< -c -o $@

$(ASCOREPRO_DSO) : | $(BUILD_DIR)/
$(ASCOREPRO_DSO) : $(ASCOREPRO_DSTDIR)%.lo : $(ASCOREPRO_SRCDIR)%.cpp
	$(CC) $(CFLAGS) $(SLFLAGS) $(INCLUDE) $(DFLAGS) $< -c -o $@

clean:
	rm -rf $(ASCOREPRO_DST) $(ASCOREPRO_DSO) libascorepro.a

lib: ascorepro
