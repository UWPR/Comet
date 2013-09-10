CXX = g++
GCC = gcc
MSTOOLKIT = ../MSToolkit
CXXFLAGS = -O3 -Wall -Wextra -static -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -I. -I$(MSTOOLKIT)/include

COMETSEARCH = Threading.o CometSearch.o CometPreprocess.o CometPostAnalysis.o CometMassSpecUtils.o CometWriteOut.o CometWriteSqt.o CometWritePepXML.o CometWritePinXML.o CometWriteTxt.o CometSearchManager.o

all:  $(COMETSEARCH)
	ar rcs libcometsearch.a $(COMETSEARCH)

clean:
	rm -f *.o libcometsearch.a

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
CometWritePinXML.o:   CometWritePinXML.cpp Common.h CometData.h CometMassSpecUtils.h CometWritePinXML.h
	${CXX} ${CXXFLAGS} CometWritePinXML.cpp -c
CometWriteTxt.o:      CometWriteTxt.cpp Common.h CometData.h CometMassSpecUtils.h CometWriteTxt.h
	${CXX} ${CXXFLAGS} CometWriteTxt.cpp -c
CometSearchManager.o:      CometSearchManager.cpp Common.h CometData.h CometMassSpecUtils.h CometSearch.h CometPostAnalysis.h CometWriteOut.h CometWriteSqt.h CometWriteTxt.h CometWritePepXML.h CometWritePinXML.h Threading.h ThreadPool.h CometSearchManager.h
	${CXX} ${CXXFLAGS} CometSearchManager.cpp -c
