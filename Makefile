include Makefile.common

COMETSEARCH =  CometSearch

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
   override CXXFLAGS += -O3         -std=c++14 -fpermissive -Wall -Wextra -Wno-char-subscripts -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D__LINUX__ -D_NOSQLITE -I$(MSTOOLKIT)/include -I$(MSTOOLKIT)/extern/expat-2.2.9/lib -I$(MSTOOLKIT)/extern/zlib-1.3.1 -I$(COMETSEARCH) -I$(ASCOREPRO)/include
else
   override CXXFLAGS += -O3 -static -std=c++14 -fpermissive -Wall -Wextra -Wno-char-subscripts -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D__LINUX__ -D_NOSQLITE -I$(MSTOOLKIT)/include -I$(MSTOOLKIT)/extern/expat-2.2.9/lib -I$(MSTOOLKIT)/extern/zlib-1.3.1 -I$(COMETSEARCH) -I$(ASCOREPRO)/include
endif

EXECNAME = comet.exe
OBJS = Comet.o
DEPS = CometSearch/CometData.h CometSearch/CometDataInternal.h CometSearch/CometPreprocess.h CometSearch/CometWriteSqt.h\
		 CometSearch/OSSpecificThreading.h CometSearch/CometMassSpecUtils.h CometSearch/CometSearch.h CometSearch/CometWritePepXML.h\
		 CometSearch/CometWriteMzIdentML.h CometSearch/CometWriteTxt.h CometSearch/Threading.h CometSearch/CometPostAnalysis.h\
		 CometSearch/CometSearchManager.h CometSearch/CometWritePercolator.h CometSearch/Common.h CometSearch/ThreadPool.h\
		 CometSearch/CombinatoricsUtils.h CometSearch/CometModificationsPermuter.h CometSearch/CometMassSpecUtils.cpp\
		 CometSearch/CometSearch.cpp CometSearch/CometWritePepXML.cpp CometSearch/CometWriteMzIdentML.cpp CometSearch/CometWriteTxt.cpp\
		 CometSearch/CometPostAnalysis.cpp CometSearch/CometSearchManager.cpp CometSearch/CometWritePercolator.cpp CometSearch/Threading.cpp\
		 CometSearch/CometPreprocess.cpp CometSearch/CometWriteSqt.cpp CometSearch/CombinatoricsUtils.cpp\
		 CometSearch/CometModificationsPermuter.cpp CometSearch/CometInterfaces.h CometSearch/CometInterfaces.cpp\
		 CometSearch/CometFragmentIndex.cpp CometSearch/CometFragmentIndex.h\
		 CometSearch/CometPeptideIndex.cpp CometSearch/CometPeptideIndex.h\
		 CometSearch/CometSpecLib.cpp CometSearch/CometSpecLib.h\
		 CometSearch/CometAlignment.cpp CometSearch/CometAlignment.h\
		 CometSearch/githubsha.h

LIBPATHS = -L$(MSTOOLKIT) -L$(COMETSEARCH) -L$(ASCOREPRO)
LIBS = -lcometsearch -lmstoolkit -lmstoolkitextern -lascorepro -lm -lpthread
ifdef MSYSTEM
   LIBS += -lws2_32
endif

comet.exe: $(OBJS)
	cd $(MSTOOLKIT) && make all
	cd $(ASCOREPRO) && make all
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
	cd $(MSTOOLKIT) ; make realclean ; cd ../CometSearch ; make clean ; cd ../$(ASCOREPRO) ; make clean

cclean:
	rm -f *.o ${EXECNAME}
	cd CometSearch ; make clean
