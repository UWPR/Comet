CXX = g++
GCC = gcc
MSTOOLKIT = ../MSToolkit
override CXXFLAGS += -O3 -Wall -Wextra -Wno-write-strings -static -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -I. -I$(MSTOOLKIT)/include

COMETSEARCH = Threading.o CometInterfaces.o CometSearch.o CometPreprocess.o CometPostAnalysis.o CometMassSpecUtils.o CometWriteOut.o CometWriteSqt.o CometWritePepXML.o CometWritePercolator.o CometWriteTxt.o CometSearchManager.o

all:  $(COMETSEARCH)
	ar rcs libcometsearch.a $(COMETSEARCH)

clean:
	rm -f *.o libcometsearch.a

Threading.o:          Threading.cpp Threading.h
	${CXX} ${CXXFLAGS} Threading.cpp -c
CometSearch.o:        CometSearch.cpp Common.h CometData.h CometDataInternal.h CometSearch.h CometInterfaces.h ThreadPool.h
	${CXX} ${CXXFLAGS} CometSearch.cpp -c
CometPreprocess.o:    CometPreprocess.cpp Common.h CometData.h CometDataInternal.h CometPreprocess.h CometInterfaces.h $(MSTOOLKIT)
	${CXX} ${CXXFLAGS} CometPreprocess.cpp -c
CometMassSpecUtils.o: CometMassSpecUtils.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometMassSpecUtils.cpp -c
CometPostAnalysis.o:  CometPostAnalysis.cpp Common.h CometData.h CometDataInternal.h ThreadPool.h CometPostAnalysis.h CometMassSpecUtils.h CometInterfaces.h CometDecoys.h
	${CXX} ${CXXFLAGS} CometPostAnalysis.cpp -c
CometWriteOut.o:      CometWriteOut.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometWriteOut.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometWriteOut.cpp -c
CometWriteSqt.o:      CometWriteSqt.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometWriteSqt.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometWriteSqt.cpp -c
CometWritePepXML.o:   CometWritePepXML.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometWritePepXML.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometWritePepXML.cpp -c
CometWritePercolator.o:   CometWritePercolator.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometWritePercolator.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometWritePercolator.cpp -c
CometWriteTxt.o:      CometWriteTxt.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometWriteTxt.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometWriteTxt.cpp -c
CometSearchManager.o:      CometSearchManager.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometSearch.h CometPostAnalysis.h CometWriteOut.h CometWriteSqt.h CometWriteTxt.h CometWritePepXML.h CometWritePercolator.h Threading.h ThreadPool.h CometSearchManager.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometSearchManager.cpp -c
CometInterfaces.o:      CometInterfaces.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometSearch.h CometPostAnalysis.h CometWriteOut.h CometWriteSqt.h CometWriteTxt.h CometWritePepXML.h CometWritePercolator.h Threading.h ThreadPool.h CometSearchManager.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometInterfaces.cpp -c
