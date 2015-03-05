CXX = g++
MSTOOLKIT = MSToolkit
COMETSEARCH = CometSearch
override CXXFLAGS += -O3 -Wall -Wextra -static -Wno-char-subscripts -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D__LINUX__ -I$(MSTOOLKIT)/include -I$(COMETSEARCH)
EXECNAME = comet.exe
OBJS = Comet.o
DEPS = CometSearch/CometData.h CometSearch/CometDataInternal.h CometSearch/CometPreprocess.h CometSearch/CometWriteOut.h CometSearch/CometWriteSqt.h CometSearch/OSSpecificThreading.h CometSearch/CometMassSpecUtils.h CometSearch/CometSearch.h CometSearch/CometWritePepXML.h CometSearch/CometWriteTxt.h CometSearch/Threading.h CometSearch/CometPostAnalysis.h CometSearch/CometSearchManager.h CometSearch/CometWritePercolator.h CometSearch/Common.h CometSearch/ThreadPool.h CometSearch/CometMassSpecUtils.cpp CometSearch/CometSearch.cpp CometSearch/CometWritePepXML.cpp CometSearch/CometWriteTxt.cpp CometSearch/CometPostAnalysis.cpp CometSearch/CometSearchManager.cpp CometSearch/CometWritePercolator.cpp CometSearch/Threading.cpp CometSearch/CometPreprocess.cpp CometSearch/CometWriteOut.cpp CometSearch/CometWriteSqt.cpp

LIBPATHS = -L$(MSTOOLKIT) -L$(COMETSEARCH)
LIBS = -lcometsearch -lmstoolkitlite -lm -lpthread
ifdef MSYSTEM
   LIBS += -lws2_32
endif

comet.exe: $(OBJS)
	cd $(MSTOOLKIT) ; make lite ; cd ../CometSearch ; make
	${CXX} $(CXXFLAGS) $(OBJS) $(LIBPATHS) $(LIBS) -o ${EXECNAME}

Comet.o: Comet.cpp $(DEPS)
	${CXX} ${CXXFLAGS} Comet.cpp -c

clean:
	rm -f *.o ${EXECNAME}
	cd $(MSTOOLKIT) ; make clean ; cd ../CometSearch ; make clean
