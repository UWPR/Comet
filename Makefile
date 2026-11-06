include Makefile.common

COMETSEARCH =  CometSearch

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
   override CXXFLAGS += -O3         -std=c++17 -fpermissive -Wall -Wextra -Wno-char-subscripts -Wno-unused-result -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D__LINUX__ -D_NOSQLITE -I$(MSTOOLKIT)/include -I$(MSTOOLKIT)/extern/expat-2.2.9/lib -I$(MSTOOLKIT)/extern/zlib-1.2.11 -I$(COMETSEARCH) -I$(ASCOREPRO)/include
else
   override CXXFLAGS += -O3 -static -std=c++17 -fpermissive -Wall -Wextra -Wno-char-subscripts -Wno-unused-result -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D__LINUX__ -D_NOSQLITE -I$(MSTOOLKIT)/include -I$(MSTOOLKIT)/extern/expat-2.2.9/lib -I$(MSTOOLKIT)/extern/zlib-1.2.11 -I$(COMETSEARCH) -I$(ASCOREPRO)/include
endif

EXECNAME = comet.exe
OBJS = Comet.o

# Automatic header dependencies: each compile writes a .d file listing every header
# it included (-MMD: user headers only; -MP: a phony rule per header so a deleted
# header does not break the build), and the .d files are included at the bottom.
# A change to any header therefore recompiles exactly the objects that include it,
# directly or transitively -- no hand-maintained header lists. CometSearch/,
# AScorePro/ and MSToolkit/ do the same in their own Makefiles.
DEPFLAGS = -MMD -MP

LIBPATHS = -L$(MSTOOLKIT) -L$(COMETSEARCH) -L$(ASCOREPRO)
LIBS = -lcometsearch -lmstoolkit -lmstoolkitextern -lascorepro -lm -lpthread
ifdef MSYSTEM
   LIBS += -lws2_32 -lpsapi
endif

LIBCOMETSEARCH = $(COMETSEARCH)/libcometsearch.a
LIBASCOREPRO = $(ASCOREPRO)/libascorepro.a

# Everything a sub-make's objects can depend on. A change to any of these makes the
# library out of date *here*, so the sub-make is entered; the sub-make's own .d
# files then decide which objects actually recompile. CometSearch's objects include
# headers from its subdirectories (core/, search/, threading/) and from AScorePro
# and MSToolkit, so those are listed too.
COMETSEARCH_FILES = $(wildcard $(COMETSEARCH)/*.cpp $(COMETSEARCH)/*.h $(COMETSEARCH)/*.hpp \
                               $(COMETSEARCH)/*/*.cpp $(COMETSEARCH)/*/*.h $(COMETSEARCH)/*/*.hpp \
                               $(ASCOREPRO)/include/*.h $(MSTOOLKIT)/include/*.h)
ASCOREPRO_FILES = $(wildcard $(ASCOREPRO)/*.cpp $(ASCOREPRO)/include/*.h)

UNITTEST_BIN = tests/unit/test_comet_internal
UNITTEST_SRC = tests/unit/TestCometSearchAndPreprocess.cpp tests/unit/TestModificationsPermuter.cpp

comet.exe: $(OBJS) $(LIBCOMETSEARCH) $(LIBASCOREPRO)
ifeq ($(UNAME_S),Darwin)
	${CXX} $(OBJS) -headerpad_max_install_names -o ${EXECNAME} $(CXXFLAGS) $(LIBPATHS) $(LIBS)
else
	${CXX} $(OBJS) -o ${EXECNAME} $(CXXFLAGS) $(LIBPATHS) $(LIBS)
endif

# MSToolkit's build extracts/builds expat and zlib under MSToolkit/extern/,
# whose generated headers Comet.cpp and CometSearch both #include (via
# MSReader.h/mzParser.h) -- it must run before either compiles, not just
# before the final comet.exe link, or a from-scratch checkout fails with
# "expat.h: No such file or directory". It is an ORDER-ONLY prerequisite
# (after the '|') of both: under 'make -j' it then runs exactly once before
# either starts, instead of once from each recipe concurrently (two unzips
# racing in MSToolkit/extern/ -- "cannot create expat-2.2.9: File exists"),
# and its run never by itself marks Comet.o or libcometsearch.a out of date;
# MSToolkit header changes reach them through the .d files.
.PHONY: mstoolkit
mstoolkit:
	cd $(MSTOOLKIT) && $(MAKE) all

$(LIBCOMETSEARCH): $(COMETSEARCH_FILES) | mstoolkit
	cd $(COMETSEARCH) && $(MAKE)

$(LIBASCOREPRO): $(ASCOREPRO_FILES)
	cd $(ASCOREPRO) && $(MAKE) all

Comet.o: Comet.cpp | mstoolkit
	${CXX} ${CXXFLAGS} ${DEPFLAGS} Comet.cpp -c

# Dependency-free unit tests (tests/unit/MiniTest.h -- no gtest, no CMake).
# Builds a standalone binary linked directly against libcometsearch and
# its dependencies, then runs it.
unittest: $(LIBCOMETSEARCH) $(LIBASCOREPRO)
	${CXX} ${CXXFLAGS} $(UNITTEST_SRC) -o $(UNITTEST_BIN) $(LIBPATHS) $(LIBS)
	$(UNITTEST_BIN)

clean:
	rm -f *.o *.d ${EXECNAME} $(UNITTEST_BIN)
	cd $(MSTOOLKIT) ; make realclean ; cd ../CometSearch ; make clean ; cd ../$(ASCOREPRO) ; make clean

cclean:
	rm -f *.o *.d ${EXECNAME} $(UNITTEST_BIN)
	cd CometSearch ; make clean

-include $(OBJS:.o=.d)
