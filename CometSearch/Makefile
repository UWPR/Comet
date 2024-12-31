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
   override CXXFLAGS += -O3 -static -std=c++14 -fpermissive -Wall -Wextra -Wno-write-strings -DGITHUBSHA='"$(GITHUB_SHA)"' -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -D_NOSQLITE -D__int64=off64_t -I. -I$(MSTPATH)/include -I$(MSTPATH)/src/expat-2.2.9/lib -I$(MSTPATH)/src/zlib-1.2.11
else
   override CXXFLAGS += -O3 -static -std=c++14 -fpermissive -Wall -Wextra -Wno-write-strings -DGITHUBSHA='"$(GITHUB_SHA)"' -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -D_NOSQLITE -D__int64=off64_t -I. -I$(MSTPATH)/include -I$(MSTPATH)/src/expat-2.2.9/lib -I$(MSTPATH)/src/zlib-1.2.11

endif

COMETSEARCH = Threading.o CometInterfaces.o CometSearch.o CometPreprocess.o CometPostAnalysis.o CometMassSpecUtils.o CometWriteOut.o\
				  CometWriteSqt.o CometWritePepXML.o CometWriteMzIdentML.o CometWritePercolator.o CometWriteTxt.o CometSearchManager.o\
				  CombinatoricsUtils.o ModificationsPermuter.o CometFragmentIndex.o CometPeptideIndex.o


all:  $(COMETSEARCH)
	ar rcs libcometsearch.a $(COMETSEARCH)

clean:
	rm -f *.o libcometsearch.a

Threading.o:          Threading.cpp Threading.h
	${CXX} ${CXXFLAGS} Threading.cpp -c
CometSearch.o:        CometSearch.cpp Common.h CometData.h CometDataInternal.h CometSearch.h CometInterfaces.h ThreadPool.h CometFragmentIndex.h CometPeptideIndex.h
	${CXX} ${CXXFLAGS} CometSearch.cpp -c
CometPreprocess.o:    CometPreprocess.cpp Common.h CometData.h CometDataInternal.h CometPreprocess.h CometInterfaces.h $(MSTPATH)
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
CometWriteMzIdentML.o:   CometWriteMzIdentML.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometWriteMzIdentML.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometWriteMzIdentML.cpp -c
CometWritePercolator.o:   CometWritePercolator.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometWritePercolator.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometWritePercolator.cpp -c
CometWriteTxt.o:      CometWriteTxt.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometWriteTxt.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometWriteTxt.cpp -c
CometCheckForUpdates.o:   CometCheckForUpdates.cpp Common.h CometCheckForUpdates.h
	${CXX} ${CXXFLAGS} CometCheckForUpdates.cpp -c
CometSearchManager.o:     CometSearchManager.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometSearch.h CometPostAnalysis.h CometWriteOut.h CometWriteSqt.h CometWriteTxt.h CometWritePepXML.h CometWriteMzIdentML.h CometWritePercolator.h Threading.h ThreadPool.h CometSearchManager.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometSearchManager.cpp -c
CometInterfaces.o:      CometInterfaces.cpp Common.h CometData.h CometDataInternal.h CometMassSpecUtils.h CometSearch.h CometPostAnalysis.h CometWriteOut.h CometWriteSqt.h CometWriteTxt.h CometWritePepXML.h CometWritePercolator.h Threading.h ThreadPool.h CometSearchManager.h CometInterfaces.h
	${CXX} ${CXXFLAGS} CometInterfaces.cpp -c
CombinatoricsUtils.o:   CombinatoricsUtils.cpp CombinatoricsUtils.h Common.h
	${CXX} ${CXXFLAGS} CombinatoricsUtils.cpp -c
ModificationsPermuter.o:   ModificationsPermuter.cpp ModificationsPermuter.h Common.h
	${CXX} ${CXXFLAGS} ModificationsPermuter.cpp -c
CometFragmentIndex.o: CometFragmentIndex.cpp Common.h CometData.h CometDataInternal.h CometSearch.h CometInterfaces.h ThreadPool.h
	${CXX} ${CXXFLAGS} CometFragmentIndex.cpp -c
CometPeptideIndex.o: CometPeptideIndex.cpp Common.h CometData.h CometDataInternal.h CometSearch.h CometInterfaces.h ThreadPool.h
	${CXX} ${CXXFLAGS} CometPeptideIndex.cpp -c
