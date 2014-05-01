#Set your paths here.
ZLIB_PATH = ./src/zLib-1.2.5
MZPARSER_PATH = ./src/mzParser
EXPAT_PATH = ./src/expat-2.0.1
SQLITE_PATH = ./src/sqlite-3.7.7.1
MST_PATH = ./src/MSToolkit
HDF5_PATH = ./src/hdf5-1.8.8

HEADER_PATH = ./include

MZPARSER = mzp.MSNumpress.o mzp.mzp_base64.o mzp.BasicSpectrum.o mzp.mzParser.o mzp.RAMPface.o mzp.saxhandler.o mzp.saxmzmlhandler.o \
	mzp.saxmzxmlhandler.o mzp.Czran.o mzp.mz5handler.o mzp.mzpMz5Config.o mzp.mzpMz5Structs.o mzp.BasicChromatogram.o mzp.PWIZface.o
MZPARSERLITE = mzp.MSNumpress.o mzp.mzp_base64_lite.o mzp.BasicSpectrum_lite.o mzp.mzParser_lite.o mzp.RAMPface_lite.o mzp.saxhandler_lite.o mzp.saxmzmlhandler_lite.o \
  mzp.saxmzxmlhandler_lite.o mzp.Czran_lite.o mzp.mz5handler_lite.o mzp.mzpMz5Config_lite.o mzp.mzpMz5Structs_lite.o mzp.BasicChromatogram_lite.o mzp.PWIZface_lite.o
EXPAT = xmlparse.o xmlrole.o xmltok.o
ZLIB = adler32.o compress.o crc32.o deflate.o inffast.o inflate.o infback.o inftrees.o trees.o uncompr.o zutil.o
MSTOOLKIT = Spectrum.o MSObject.o
READER = MSReader.o
READERLITE = MSReaderLite.o
SQLITE = sqlite3.o 
HDF5 = H5AbstractDs.o H5ArrayType.o H5AtomType.o H5Attribute.o H5CommonFG.o H5CompType.o H5DataSet.o H5DataSpace.o H5DataType.o \
	H5DcreatProp.o H5DxferProp.o H5EnumType.o H5Exception.o H5FaccProp.o H5FcreatProp.o H5File.o H5FloatType.o H5Group.o H5IdComponent.o \
	H5IntType.o H5Library.o H5Object.o H5PredType.o H5PropList.o H5StrType.o H5VarLenType.o \
	H5Abtree2.o H5Dfill.o H5Ffake.o H5Groot.o H5L.o H5Omessage.o H5Rdeprec.o H5Tfloat.o H5A.o H5Dint.o H5Fio.o H5Gstab.o H5Lexternal.o H5Omtime.o H5RS.o H5timer.o \
	H5AC.o H5Dio.o H5FL.o H5Gtest.o H5lib_settings.o H5Oname.o H5Sall.o H5Tinit.o H5Adense.o H5Dlayout.o H5Fmount.o H5Gtraverse.o H5make_libsettings.o H5Onull.o H5S.o H5Tnative.o \
	H5Adeprec.o H5Dmpio.o H5Fmpi.o H5HFbtree2.o   H5MFaggr.o H5Opline.o H5Sdbg.o H5Toffset.o H5Aint.o H5Doh.o H5FO.o H5HF.o H5MF.o H5Orefcount.o  H5Shyper.o H5Toh.o \
	H5Atest.o H5Dscatgath.o H5Fquery.o H5HFcache.o H5MFdbg.o H5Osdspace.o H5SL.o H5Topaque.o H5B2.o H5Dselect.o H5FS.o H5HFdbg.o H5MFsection.o H5Oshared.o H5SMbtree2.o H5Torder.o \
	H5B2cache.o H5Dtest.o H5FScache.o H5HFdblock.o H5MM.o H5Oshmesg.o H5SM.o H5Tpad.o H5B2dbg.o H5E.o H5FSdbg.o H5HFdtable.o H5MP.o H5Ostab.o H5SMcache.o H5Tprecis.o \
	H5B2hdr.o H5Edeprec.o H5Fsfile.o H5HFhdr.o H5MPtest.o H5Otest.o H5SMmessage.o  H5trace.o H5B2int.o H5Eint.o H5FSsection.o H5HFhuge.o H5Oainfo.o H5Ounknown.o H5Smpio.o H5TS.o \
	H5B2stat.o H5Faccum.o H5FSstat.o H5HFiblock.o H5Oalloc.o H5Pacpl.o H5SMtest.o H5Tstrpad.o H5B2test.o H5F.o H5FStest.o H5HFiter.o H5Oattr.o H5P.o H5Snone.o H5Tvisit.o \
	H5B.o H5Fcwfs.o H5Fsuper.o H5HFman.o H5Oattribute.o H5Pdapl.o H5Spoint.o H5Tvlen.o H5Bcache.o H5Fdbg.o H5Fsuper_cache.o H5HFsection.o H5Obogus.o H5Pdcpl.o H5Sselect.o H5V.o \
	H5Bdbg.o H5FD.o H5Ftest.o H5HFspace.o H5Obtreek.o H5Pdeprec.o H5ST.o H5WB.o H5.o H5FDcore.o H5Gbtree2.o H5HFstat.o H5O.o H5Pdxpl.o H5Stest.o H5Z.o \
	H5C.o H5FDdirect.o H5G.o H5HFtest.o H5Ocache.o H5Pfapl.o H5system.o H5Zdeflate.o H5checksum.o H5FDfamily.o H5Gcache.o H5HFtiny.o H5Ochunk.o H5Pfcpl.o H5Tarray.o H5Zfletcher32.o \
	H5CS.o H5FDint.o H5Gcompact.o H5HG.o H5Ocont.o H5Pfmpl.o H5Tbit.o H5Znbit.o H5dbg.o H5Gdense.o H5HGcache.o H5Ocopy.o H5Pgcpl.o H5T.o H5Zscaleoffset.o \
	H5Dbtree.o H5FDmpi.o H5Gdeprec.o H5HGdbg.o H5Odbg.o H5Pint.o H5Tcommit.o H5Zshuffle.o H5D.o H5FDmpio.o H5Gent.o H5HGquery.o H5Odrvinfo.o H5Plapl.o H5Tcompound.o H5Zszip.o \
	H5Dchunk.o H5FDmpiposix.o H5Gint.o H5HL.o H5Odtype.o H5Plcpl.o H5Tconv.o H5Ztrans.o H5Dcompact.o H5FDmulti.o H5Glink.o H5HLcache.o H5Oefl.o H5Pocpl.o H5Tcset.o \
	H5Dcontig.o H5Gloc.o H5HLdbg.o H5Ofill.o H5Pocpypl.o H5Tdbg.o H5Ddbg.o H5FDspace.o H5Gname.o H5HLint.o H5Oginfo.o H5Pstrcpl.o H5Tdeprec.o \
	H5Ddeprec.o H5Gnode.o H5HP.o H5Olayout.o H5Ptest.o H5Tenum.o H5Defl.o H5FDwindows.o H5Gobj.o H5I.o H5Olinfo.o H5R.o H5Tfields.o \
	H5detect.o H5Fefc.o H5Goh.o H5Itest.o H5Olink.o H5RC.o H5Tfixed.o H5FDlog.o H5FDsec2.o H5FDstdio.o

CC = g++
GCC = gcc
NOSQLITE = -D_NOSQLITE
#MZ5 = -DMST_MZ5 -DMZP_MZ5
MZ5 = 

H5_CPPFLAGS = -O3 -static -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_BSD_SOURCE  -D_POSIX_C_SOURCE=199506L   -DNDEBUG -UH5_DEBUG_API
CFLAGS = -O3 -static -I. -I$(HEADER_PATH) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DGCC -DHAVE_EXPAT_CONFIG_H
LIBS = -lm -lpthread -ldl

all:  $(ZLIB) $(MZPARSER) $(MZPARSERLITE) $(MSTOOLKIT) $(READER) $(READERLITE) $(EXPAT)
	ar rcs libmstoolkitlite.a $(ZLIB) $(EXPAT) $(MZPARSERLITE) $(MSTOOLKIT) $(READERLITE)
#	ar rcs libmstoolkit.a $(ZLIB) $(EXPAT) $(MZPARSER) $(MSTOOLKIT) $(READER) $(SQLITE) $(HDF5)
#	$(CC) $(CFLAGS) MSTDemo.cpp -L. -lmstoolkitlite -o MSTDemo
#	$(CC) $(CFLAGS) -I./include MSSingleScan/MSSingleScan.cpp -L. -lmstoolkitlite -o msSingleScan
#	$(CC) $(CFLAGS) MSConvertFile.cpp -L. -lmstoolkitlite -o MSConvertFile

lite: $(ZLIB) $(MZPARSERLITE) $(MSTOOLKIT) $(READERLITE) $(EXPAT)
	ar rcs libmstoolkitlite.a $(ZLIB) $(EXPAT) $(MZPARSERLITE) $(MSTOOLKIT) $(READERLITE)

clean:
	rm -f *.o libmstoolkitlite.a libmstoolkit.a msSingleScan

# HDF5 objects
H5%.o : $(HDF5_PATH)/H5%.cpp
	$(CC) $(H5_CPPFLAGS) -I. -I$(HEADER_PATH) $< -c

H5%.o : $(HDF5_PATH)/H5%.c
	$(GCC) $(H5_CPPFLAGS) -I. -I$(HEADER_PATH) $< -c

H5.o : $(HDF5_PATH)/H5.c
	$(GCC) $(H5_CPPFLAGS) -I. -I$(HEADER_PATH) $(HDF5_PATH)/H5.c -c
	
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
mzp.%.o : $(MZPARSER_PATH)/%.cpp
	$(CC) $(CFLAGS) $(MZ5) $< -c -o $@

#mzParserLite objects
mzp.%_lite.o : $(MZPARSER_PATH)/%.cpp
	$(CC) $(CFLAGS) $< -c -o $@


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
	$(CC) $(CFLAGS) $(MZ5) $(MST_PATH)/MSReader.cpp -c

MSReaderLite.o : $(MST_PATH)/MSReader.cpp
	$(CC) $(CFLAGS) $(NOSQLITE) $(MST_PATH)/MSReader.cpp -c -o MSReaderLite.o

MSObject.o : $(MST_PATH)/MSObject.cpp
	$(CC) $(CFLAGS) $(MST_PATH)/MSObject.cpp -c



