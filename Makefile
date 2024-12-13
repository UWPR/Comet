include Makefile.common

COMETSEARCH =  CometSearch

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
   override CXXFLAGS += -O3         -std=c++14 -fpermissive -Wall -Wextra -Wno-char-subscripts -DGITHUBSHA='"$(GITHUB_SHA)"' -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D__LINUX__ -D_NOSQLITE -I$(MSTOOLKIT)/include -I$(MSTOOLKIT)/src/expat-2.2.9/lib -I$(MSTOOLKIT)/src/zlib-1.2.11 -I$(COMETSEARCH)
else
   override CXXFLAGS += -O3 -static -std=c++14 -fpermissive -Wall -Wextra -Wno-char-subscripts -DGITHUBSHA='"$(GITHUB_SHA)"' -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D__LINUX__ -D_NOSQLITE -I$(MSTOOLKIT)/include -I$(MSTOOLKIT)/src/expat-2.2.9/lib -I$(MSTOOLKIT)/src/zlib-1.2.11 -I$(COMETSEARCH)
endif

EXECNAME = comet.exe
OBJS = Comet.o
DEPS = CometSearch/CometData.h CometSearch/CometDataInternal.h CometSearch/CometPreprocess.h CometSearch/CometWriteOut.h CometSearch/CometWriteSqt.h\
		 CometSearch/OSSpecificThreading.h CometSearch/CometMassSpecUtils.h CometSearch/CometSearch.h CometSearch/CometWritePepXML.h\
		 CometSearch/CometWriteMzIdentML.h CometSearch/CometWriteTxt.h CometSearch/Threading.h CometSearch/CometPostAnalysis.h\
		 CometSearch/CometSearchManager.h CometSearch/CometWritePercolator.h CometSearch/Common.h CometSearch/ThreadPool.h\
		 CometSearch/CombinatoricsUtils.h CometSearch/ModificationsPermuter.h CometSearch/CometMassSpecUtils.cpp\
		 CometSearch/CometSearch.cpp CometSearch/CometWritePepXML.cpp CometSearch/CometWriteMzIdentML.cpp CometSearch/CometWriteTxt.cpp\
		 CometSearch/CometPostAnalysis.cpp CometSearch/CometSearchManager.cpp CometSearch/CometWritePercolator.cpp CometSearch/Threading.cpp\
		 CometSearch/CometPreprocess.cpp CometSearch/CometWriteOut.cpp CometSearch/CometWriteSqt.cpp CometSearch/CombinatoricsUtils.cpp\
		 CometSearch/ModificationsPermuter.cpp CometSearch/CometInterfaces.h CometSearch/CometInterfaces.cpp\
		 CometSearch/CometFragmentIndex.cpp CometSearch/CometFragmentIndex.h\
		 CometSearch/CometPeptideIndex.cpp CometSearch/CometPeptideIndex.h

LIBPATHS = -L$(MSTOOLKIT) -L$(COMETSEARCH)
LIBS = -lcometsearch -lmstoolkitlite -lm -lpthread 
ifdef MSYSTEM
   LIBS += -lws2_32
endif

comet.exe: $(OBJS)
	cd $(MSTOOLKIT) && make all
	cd $(COMETSEARCH) && make

ifeq ($(UNAME_S),Darwin)
	${CXX} $(OBJS) -headerpad_max_install_names -o ${EXECNAME} $(CXXFLAGS) $(LIBPATHS) $(LIBS)
else
	${CXX} $(OBJS) -o ${EXECNAME} $(CXXFLAGS) $(LIBPATHS) $(LIBS)
endif

Comet.o: Comet.cpp $(DEPS)
	${CXX} ${CXXFLAGS} Comet.cpp -c

clean:
	rm -f *.o ${EXECNAME}
	cd $(MSTOOLKIT) ; make realclean ; cd ../CometSearch ; make clean

cclean:
	rm -f *.o ${EXECNAME}
	cd CometSearch ; make clean

mstclean:
	cd $(MSTOOLKIT)/src ; rm -rf expat-2.2.9 zlib-1.2.11/ ; tar xzf expat-2.2.9.tar.gz  ; unzip zlib1211.zip
	cd $(MSTOOLKIT)/include ; rm -f expat.h expat_external.h zconf.h zlib.h
