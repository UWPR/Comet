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
   override CXXFLAGS += -O3 -static -std=c++14 -fpermissive -Wall -Wextra -Wno-write-strings -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -D_NOSQLITE -D__int64=off64_t -I. -I$(MSTPATH)/include -I$(MSTPATH)/src/expat-2.2.9/lib -I$(MSTPATH)/src/zlib-1.2.11 -I../$(ASCOREPRO)/include
else
   override CXXFLAGS += -O3 -static -std=c++14 -fpermissive -Wall -Wextra -Wno-write-strings -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -D_NOSQLITE -D__int64=off64_t -I. -I$(MSTPATH)/include -I$(MSTPATH)/src/expat-2.2.9/lib -I$(MSTPATH)/src/zlib-1.2.11 -I../$(ASCOREPRO)/include
endif

OBJDIR = obj
COMETSEARCH_SRC = Threading CometInterfaces CometSearch CometPreprocess CometPostAnalysis CometMassSpecUtils CometWriteOut \
				  CometWriteSqt CometWritePepXML CometWriteMzIdentML CometWritePercolator CometWriteTxt CometSearchManager \
				  CombinatoricsUtils CometModificationsPermuter CometFragmentIndex CometPeptideIndex CometSpecLib CometAlignment

COMETSEARCH_OBJ = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(COMETSEARCH_SRC)))

all: $(OBJDIR) $(COMETSEARCH_OBJ)
	ar rcs libcometsearch.a $(COMETSEARCH_OBJ)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.cpp %.h | $(OBJDIR)
	${CXX} ${CXXFLAGS} $< -c -o $@

# Add specific dependency rules for object files that require multiple headers
$(OBJDIR)/CometSearch.o: CometSearch.cpp Common.h CometData.h CometDataInternal.h CometSearch.h CometInterfaces.h ThreadPool.h CometFragmentIndex.h CometPeptideIndex.h | $(OBJDIR)
	${CXX} ${CXXFLAGS} CometSearch.cpp -c -o $@

$(OBJDIR)/CometPreprocess.o: CometPreprocess.cpp Common.h CometData.h CometDataInternal.h CometPreprocess.h CometInterfaces.h $(MSTPATH) | $(OBJDIR)
	${CXX} ${CXXFLAGS} CometPreprocess.cpp -c -o $@

$(OBJDIR)/CometMassSpecUtils.o: CometMassSpecUtils.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometInterfaces.h | $(OBJDIR)
	${CXX} ${CXXFLAGS} CometMassSpecUtils.cpp -c -o $@

clean:
	rm -rf $(OBJDIR) *.a
