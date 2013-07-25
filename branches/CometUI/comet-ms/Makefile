CXX = g++
MSTOOLKIT = MSToolkit
COMETSEARCH = CometSearch
CXXFLAGS = -O3 -static -Wall -Wno-char-subscripts -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I$(MSTOOLKIT)/include -I$(COMETSEARCH)
EXECNAME = comet.exe
OBJS = Comet.o
DEPS = 
LIBS = -lm -lmstoolkitlite -lpthread -lcometsearch


comet.exe: $(OBJS)
	cd MSToolkit ; make ; cd CometSearch; make
	${CXX} $(CXXFLAGS) $(OBJS) -L$(MSTOOLKIT) -L$(COMETSEARCH) $(LIBS) -o ${EXECNAME}

Comet.o:              Comet.cpp $(DEPS)
	${CXX} ${CXXFLAGS} Comet.cpp -c

clean:
	rm -f *.o ${EXECNAME}
	cd MSToolkit ; make clean
