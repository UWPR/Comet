/*
MIT License

Copyright (c) 2023 University of Washington's Proteomics Resource

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef _WIN32
//socket
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <direct.h>
#include <errno.h>
#define STRCMP_IGNORE_CASE(a,b) _strcmpi(a,b)
#include <io.h>
typedef __int64 comet_fileoffset_t;
#define comet_fseek(handle, offset, whence) _fseeki64(handle, offset, whence)
#define comet_ftell(handle) _ftelli64(handle)
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#define STRCMP_IGNORE_CASE(a,b) strcasecmp(a,b)
#ifdef __APPLE__
#define off64_t off_t
#define fseeko64 fseeko
#define ftello64 ftello
#endif
typedef off64_t comet_fileoffset_t;
#define comet_fseek(handle, offset, whence) fseeko64(handle, offset, whence)
#define comet_ftell(handle) ftello64(handle)
#endif

using namespace std;

#include "ThreadPool.h"
#include "MSReader.h"
#include "Spectrum.h"
#include "MSObject.h"

#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>
#include <utility>
#include <set>
#include <cfloat>
#include <iostream>

#ifndef GITHUBSHA          // value passed thru at compile time
   #define GITHUBSHA ""
#endif

#define comet_version   "2023.02 rev. 0"
#define copyright "(c) University of Washington"
extern string g_sCometVersion;   // version string including git hash

// Redefined how the bin offset is interpreted and applied.  The valid range for the offset is
// now between 0.0 and 1.0 and scales to the binWidth.
#define BIN(dMass) (int)((dMass)*g_staticParams.dInverseBinWidth + g_staticParams.dOneMinusBinOffset)

#define isEqual(x, y) (std::abs(x-y) <= ( (std::abs(x) > std::abs(y) ? std::abs(y) : std::abs(x)) * FLT_EPSILON))

using namespace MSToolkit;
#ifdef CRUX
#define logout(szString) cerr << szString
#define logerr(szString) cerr << szString
#else
#define logout(szString) fputs(szString, stdout)
#define logerr(szString) fputs(szString, stderr)
#endif

#endif // _COMMON_H_
