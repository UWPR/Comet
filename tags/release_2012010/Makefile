CXX = g++
MSTOOLKIT = MSToolkit
CXXFLAGS = -O3 -static -Wall -Wno-char-subscripts -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I$(MSTOOLKIT)/include
EXECNAME = comet.exe
OBJS = Comet.o Threading.o CometSearch.o CometPreprocess.o CometPostAnalysis.o CometMassSpecUtils.o CometWriteOut.o CometWriteSqt.o CometWritePepXML.o
DEPS = Common.h OSSpecificThreading.h CometData.h CometSearch.h CometPreprocess.h ThreadPool.h CometPostAnalysis.h CometMassSpecUtils.h
LIBS = -lm -lmstoolkitlite -lpthread


comet.exe: $(OBJS)
	${CXX} $(CXXFLAGS) $(OBJS) -L$(MSTOOLKIT) $(LIBS) -o ${EXECNAME}

Comet.o:              Comet.cpp $(DEPS)
	${CXX} ${CXXFLAGS} Comet.cpp -c
Threading.o:          Threading.cpp Threading.h
	${CXX} ${CXXFLAGS} Threading.cpp -c
CometSearch.o:        CometSearch.cpp Common.h CometData.h CometSearch.h ThreadPool.h
	${CXX} ${CXXFLAGS} CometSearch.cpp -c
CometPreprocess.o:    CometPreprocess.cpp Common.h CometData.h CometPreprocess.h $(MSTOOLKIT)
	${CXX} ${CXXFLAGS} CometPreprocess.cpp -c
CometMassSpecUtils.o: CometMassSpecUtils.cpp Common.h CometData.h CometMassSpecUtils.h
	${CXX} ${CXXFLAGS} CometMassSpecUtils.cpp -c
CometPostAnalysis.o:  CometPostAnalysis.cpp Common.h CometData.h ThreadPool.h CometPostAnalysis.h CometMassSpecUtils.h
	${CXX} ${CXXFLAGS} CometPostAnalysis.cpp -c
CometWriteOut.o:      CometWriteOut.cpp Common.h CometData.h CometMassSpecUtils.h CometWriteOut.h
	${CXX} ${CXXFLAGS} CometWriteOut.cpp -c
CometWriteSqt.o:      CometWriteSqt.cpp Common.h CometData.h CometMassSpecUtils.h CometWriteSqt.h
	${CXX} ${CXXFLAGS} CometWriteSqt.cpp -c
CometWritePepXML.o:   CometWritePepXML.cpp Common.h CometData.h CometMassSpecUtils.h CometWritePepXML.h
	${CXX} ${CXXFLAGS} CometWritePepXML.cpp -c

clean:
	rm -f *.o ${EXECNAME}
