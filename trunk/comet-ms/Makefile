CXX = g++
MSTOOLKIT = MSToolkit
COMETSEARCH = CometSearch
CXXFLAGS = -O3 -Wall -Wno-char-subscripts -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I$(MSTOOLKIT)/include -I$(COMETSEARCH)
EXECNAME = comet.exe
OBJS = Comet.o
DEPS = 
LIBPATHS = -L$(MSTOOLKIT) -L$(COMETSEARCH)
LIBS = -lcometsearch -lmstoolkitlite -lm -lpthread


comet.exe: $(OBJS)
	cd MSToolkit ; make ; cd ../CometSearch; make
	${CXX} $(CXXFLAGS) $(OBJS) $(LIBPATHS) $(LIBS) -o ${EXECNAME}

Comet.o:              Comet.cpp $(DEPS)
	${CXX} ${CXXFLAGS} Comet.cpp -c

clean:
	rm -f *.o ${EXECNAME}
	cd MSToolkit ; make clean; cd ../CometSearch; make clean
