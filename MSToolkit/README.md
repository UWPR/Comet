# MSToolkit

The MSToolkit is a light-weight C++ library for reading, [sometimes] writing\*, and manipulating mass spectrometry data. The MSToolkit is easily linked to virtually any C++ algorithm for simple, fast file reading and analysis.

### Version 2.0.0, September 13 2024

### Supported File Formats
  * *mzML* including internal compression (zlib and numpress) and external compression (.mzML.gz) _read-only_
  * *mzMLb* _read-only_ (Requires HDF5)
  * *mz5* _read-only_ (Requires HDF5)
  * *mzXML* including internal compression (zlib) and external compression (.mzXML.gz) _read-only_
  * *mgf* _read/write_
  * *ms1* _read/write_
  * *ms2* _read/write_
  * *bms1* _read/write_
  * *bms2* _read/write_
  * *cms1* _read/write_
  * *cms2* _read/write_
  * *RAW* Thermo proprietary file format (Windows only, requires Xcalibur/MSFileReader) _read-only_
    

### Simple Interface
  * Open any file format from a single function call.
  * Store any spectrum in a simple, comprehensive data structure.
  * Sequential or random-access file reading.


### Easy Integration
  * All headers included from a single location.
  * Single library file easily linked by the compiler.


### Compiling
A few hints:
 * On Windows, a Visual Studio (2019) solution has been provided.
   * Add _NO_THERMORAW to build without Thermo file support in Windows, especially if you do not have these vendor .dlls installed. This is not necessary in GNU/Linux, where Thermo support is disabled by default.
   * MAKEFILE.nmake is for MSVC builds without creating your own solution file. Use the x64 Native Tools Command Prompt in VS (tested on VS2019) and type nmake /f MAKEFILE.nmake all
 * On GNU/Linux, try "make all" to build the library.
 * CMake is currently broken (just not updated to the new structuring).
 * To support .mzMLb and .mz5, HDF5 must be installed. Enable these formats by opening the Makefile and setting HDF5 and HDF5_DIR at the top of the file.
 * 3rd party libraries are still provided and compiled in a separate library (MSToolkitExtern.lib).


### Linking
To use MSToolkit in your project, add the /include folder in your project and link to MSToolkit.lib. MSToolkit requires Expat and zlib, and you may link to your own builds of these libraries if you wish. If you do not have them, also add the /include/extern folder in your project and link to MSToolkitExtern.lib. MSToolkit can be statically linked in your project, but .dll files are also built for those who prefer that route.

   
### License
Code written for the MSToolkit uses the Apache License, Version 2.0. All 3rd party software included in the MSToolkit library retains its original license.


  _\* Note: If you want to write open standard files such as .mzML, try fishing around in my other repos to see what I have. I'll formally link them here once I find the
  time to document the tools. But if you're comfortable with my coding and API styles, you'll probably be able to get started without waiting on me._
