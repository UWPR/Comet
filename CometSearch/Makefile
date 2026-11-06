include ../Makefile.common

# Need to handle whether $(MSTOOLKIT) is defined as absolute or relative path
# from parent directory and set MSTPATH accordingly
IS_ROOT    := $(if $(patsubst /%,,$(MSTOOLKIT)),,yes)
IS_HOME    := $(if $(patsubst ~%,,$(MSTOOLKIT)),,yes)
IS_NETWORK := $(if $(patsubst \\\\%,,$(MSTOOLKIT)),,yes)
IS_DRIVE   := $(foreach d,A B C D E F G H I J K L M N O P Q R S T U V Q X Y Z,$(if $(patsubst $(d):/%,,$(MSTOOLKIT)),,yes))
ifeq ($(strip $(IS_ROOT)$(IS_HOME)$(IS_NETWORK)$(IS_DRIVE)),yes)
MSTPATH = $(MSTOOLKIT)
else
MSTPATH = ../$(MSTOOLKIT)
endif

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
   override CXXFLAGS += -O3 -std=c++17 -fpermissive -Wall -Wextra -Wno-write-strings -Wno-unknown-pragmas -Wno-char-subscripts -Wno-unused-result -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -D_NOSQLITE -D__int64=off64_t -I. -I$(MSTPATH)/include -I$(MSTPATH)/extern/expat-2.2.9/lib -I$(MSTPATH)/extern/zlib-1.2.11 -I../$(ASCOREPRO)/include
else
   override CXXFLAGS += -O3 -std=c++17 -fpermissive -Wall -Wextra -Wno-write-strings -Wno-unknown-pragmas -Wno-char-subscripts -Wno-unused-result -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -D_NOSQLITE -D__int64=off64_t -I. -I$(MSTPATH)/include -I$(MSTPATH)/extern/expat-2.2.9/lib -I$(MSTPATH)/extern/zlib-1.2.11 -I../$(ASCOREPRO)/include
endif

# Automatic header dependencies (gcc/clang): every compile writes obj/<name>.d
# listing each header it included (-MMD: user headers only; -MP: a phony rule per
# header so a deleted header does not break the build). The .d files are included
# at the bottom, so a change to any header a source includes -- directly or
# transitively, here, in core/, search/, threading/, ../AScorePro/include or
# ../MSToolkit/include -- recompiles exactly the objects that include it. No
# hand-maintained header lists.
DEPFLAGS := -MMD -MP

OBJDIR = obj
COMETSEARCH_SRC = Threading CometInterfaces CometSearch CometPreprocess CometPostAnalysis CometMassSpecUtils \
					CometWriteSqt CometWritePepXML CometWriteMzIdentML CometWritePercolator CometWriteTxt CometSearchManager \
					CombinatoricsUtils CometModificationsPermuter CometFragmentIndex CometPeptideIndex CometSpecLib CometAlignment

THREADING_SRC = threading/SearchMemoryPool

SEARCH_SRC = search/SearchUtils search/FiStrategy search/FastaStrategy search/PiStrategy search/Pipeline

COMETSEARCH_OBJ = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(COMETSEARCH_SRC))) \
                  $(addprefix $(OBJDIR)/, $(addsuffix .o, $(THREADING_SRC))) \
                  $(addprefix $(OBJDIR)/, $(addsuffix .o, $(SEARCH_SRC)))

all: libcometsearch.a

libcometsearch.a: $(COMETSEARCH_OBJ)
	ar rcs libcometsearch.a $(COMETSEARCH_OBJ)

# One rule for every object, including those under obj/search/ and obj/threading/:
# the header prerequisites come from the .d file written by the previous compile
# of the same object (none on a first build, when the object is missing anyway).
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	${CXX} ${CXXFLAGS} ${DEPFLAGS} $< -c -o $@

clean:
	rm -rf $(OBJDIR) *.a

-include $(COMETSEARCH_OBJ:.o=.d)
