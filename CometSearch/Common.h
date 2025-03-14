// Copyright 2023 Jimmy Eng
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


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
#define PATH_MAX _MAX_PATH
#define realpath(N,R) _fullpath((R),(N),PATH_MAX)
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
#include <functional>

#ifndef GITHUBSHA          // value passed thru at compile time
   #define GITHUBSHA ""
#endif

#define comet_version   "2025.01 rev. 1"
#define copyright "(c) University of Washington"
extern string g_sCometVersion;   // version string including git hash

// Redefined how the bin offset is interpreted and applied.  The valid range for the offset is
// now between 0.0 and 1.0 and scales to the binWidth.
#define BIN(dMass) (int)((dMass)*g_staticParams.dInverseBinWidth + g_staticParams.dOneMinusBinOffset)

#define isEqual(x, y) (std::abs(x-y) <= ( (std::abs(x) > std::abs(y) ? std::abs(y) : std::abs(x)) * FLT_EPSILON))

#define cometbitset(byte, nbit)   ((byte) |=  (1<<(nbit)))  // https://www.codementor.io/@hbendali/c-c-macro-bit-operations-ztrat0et6
#define cometbitclear(byte, nbit) ((byte) &= ~(1<<(nbit)))
#define cometbitflip(byte, nbit)  ((byte) ^=  (1<<(nbit)))
#define cometbitcheck(byte, nbit) ((byte) &   (1<<(nbit)))


using namespace MSToolkit;
#ifdef CRUX
#define logout(szString) cerr << szString
#define logerr(szString) cerr << szString
#else
#define logout(szString) fputs(szString, stdout)
#define logerr(szString) fputs(szString, stderr)
#endif

#endif // _COMMON_H_
