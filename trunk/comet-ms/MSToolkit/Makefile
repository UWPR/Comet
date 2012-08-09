#Set your paths here.
ZLIB_PATH = ./src/zLib-1.2.5
MZPARSER_PATH = ./src/mzParser
EXPAT_PATH = ./src/expat-2.0.1
SQLITE_PATH = ./src/sqlite-3.7.7.1
MST_PATH = ./src/MSToolkit

HEADER_PATH = ./include

MZPARSER = mzp_base64.o BasicSpectrum.o mzParser.o RAMPface.o saxhandler.o saxmzmlhandler.o saxmzxmlhandler.o Czran.o
EXPAT = xmlparse.o xmlrole.o xmltok.o
ZLIB = adler32.o compress.o crc32.o deflate.o inffast.o inflate.o infback.o inftrees.o trees.o uncompr.o zutil.o
MSTOOLKIT = Spectrum.o MSObject.o
READER = MSReader.o
READERLITE = MSReaderLite.o
SQLITE = sqlite3.o 

CC = g++
GCC = gcc
NOSQLITE = -D_NOSQLITE

CFLAGS = -O3 -static -I. -I$(HEADER_PATH) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -DHAVE_EXPAT_CONFIG_H
LIBS = -lm -lpthread -ldl

all:  $(ZLIB) $(MZPARSER) $(MSTOOLKIT) $(READER) $(READERLITE) $(EXPAT) $(SQLITE)
	ar rcs libmstoolkitlite.a $(ZLIB) $(EXPAT) $(MZPARSER) $(MSTOOLKIT) $(READERLITE)
	ar rcs libmstoolkit.a $(ZLIB) $(EXPAT) $(MZPARSER) $(MSTOOLKIT) $(READER) $(SQLITE)
#	$(CC) $(CFLAGS) MSTDemo.cpp -L. -lmstoolkitlite -o MSTDemo
#	$(CC) $(CFLAGS) MSSingleScan.cpp -L. -lmstoolkitlite -o MSSingleScan
#	$(CC) $(CFLAGS) MSConvertFile.cpp -L. -lmstoolkitlite -o MSConvertFile

clean:
	rm -f *.o libmstoolkitlite.a libmstoolkit.a

# zLib objects

adler32.o : $(ZLIB_PATH)/adler32.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/adler32.c -c

compress.o : $(ZLIB_PATH)/compress.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/compress.c -c

crc32.o : $(ZLIB_PATH)/crc32.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/crc32.c -c

deflate.o : $(ZLIB_PATH)/deflate.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/deflate.c -c

inffast.o : $(ZLIB_PATH)/inffast.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/inffast.c -c

inflate.o : $(ZLIB_PATH)/inflate.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/inflate.c -c

infback.o : $(ZLIB_PATH)/infback.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/infback.c -c

inftrees.o : $(ZLIB_PATH)/inftrees.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/inftrees.c -c

trees.o : $(ZLIB_PATH)/trees.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/trees.c -c

uncompr.o : $(ZLIB_PATH)/uncompr.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/uncompr.c -c

zutil.o : $(ZLIB_PATH)/zutil.c
	$(GCC) $(CFLAGS) $(ZLIB_PATH)/zutil.c -c



#mzParser objects
mzp_base64.o : $(MZPARSER_PATH)/mzp_base64.cpp
	$(CC) $(CFLAGS) $(MZPARSER_PATH)/mzp_base64.cpp -c

BasicSpectrum.o : $(MZPARSER_PATH)/BasicSpectrum.cpp
	$(CC) $(CFLAGS) $(MZPARSER_PATH)/BasicSpectrum.cpp -c

mzParser.o : $(MZPARSER_PATH)/mzParser.cpp
	$(CC) $(CFLAGS) $(MZPARSER_PATH)/mzParser.cpp -c

RAMPface.o : $(MZPARSER_PATH)/RAMPface.cpp
	$(CC) $(CFLAGS) $(MZPARSER_PATH)/RAMPface.cpp -c

saxhandler.o : $(MZPARSER_PATH)/saxhandler.cpp
	$(CC) $(CFLAGS) $(MZPARSER_PATH)/saxhandler.cpp -c

saxmzmlhandler.o : $(MZPARSER_PATH)/saxmzmlhandler.cpp
	$(CC) $(CFLAGS) $(MZPARSER_PATH)/saxmzmlhandler.cpp -c

saxmzxmlhandler.o : $(MZPARSER_PATH)/saxmzxmlhandler.cpp
	$(CC) $(CFLAGS) $(MZPARSER_PATH)/saxmzxmlhandler.cpp -c
	
Czran.o : $(MZPARSER_PATH)/Czran.cpp
	$(CC) $(CFLAGS) $(MZPARSER_PATH)/Czran.cpp -c


#expat objects
xmlparse.o : $(EXPAT_PATH)/xmlparse.c
	$(GCC) $(CFLAGS) $(EXPAT_PATH)/xmlparse.c -c
xmlrole.o : $(EXPAT_PATH)/xmlrole.c
	$(GCC) $(CFLAGS) $(EXPAT_PATH)/xmlrole.c -c
xmltok.o : $(EXPAT_PATH)/xmltok.c
	$(GCC) $(CFLAGS) $(EXPAT_PATH)/xmltok.c -c



#SQLite object
sqlite3.o : $(SQLITE_PATH)/sqlite3.c
	$(GCC) $(CFLAGS) $(SQLITE_PATH)/sqlite3.c -c




#MSToolkit objects

Spectrum.o : $(MST_PATH)/Spectrum.cpp
	$(CC) $(CFLAGS) $(MST_PATH)/Spectrum.cpp -c

MSReader.o : $(MST_PATH)/MSReader.cpp
	$(CC) $(CFLAGS) $(MST_PATH)/MSReader.cpp -c

MSReaderLite.o : $(MST_PATH)/MSReader.cpp
	$(CC) $(CFLAGS) $(NOSQLITE) $(MST_PATH)/MSReader.cpp -c -o MSReaderLite.o

MSObject.o : $(MST_PATH)/MSObject.cpp
	$(CC) $(CFLAGS) $(MST_PATH)/MSObject.cpp -c



