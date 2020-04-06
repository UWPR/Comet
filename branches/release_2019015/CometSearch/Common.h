/*
   Copyright 2012 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <cmath>
#include <string>
#include <ctime>

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
#define unlink(fp) _unlink(fp)
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#define STRCMP_IGNORE_CASE(a,b) strcasecmp(a,b)
typedef off64_t comet_fileoffset_t;
#define comet_fseek(handle, offset, whence) fseeko64(handle, offset, whence)
#define comet_ftell(handle) ftello64(handle)
#endif

using namespace std;

#include "MSReader.h"
#include "Spectrum.h"
#include "MSObject.h"
#include <vector>
#include <set>
#include <cfloat>

#ifdef CRUX
#include <iostream>
#endif

#define comet_version   "2019.01 rev. 5"
#define copyright "(c) University of Washington"

// Redefined how the bin offset is interpreted and applied.  The valid range for the offset is
// now between 0.0 and 1.0 and scales to the binWidth.
#define BIN(dMass) (int)(dMass*g_staticParams.dInverseBinWidth + g_staticParams.dOneMinusBinOffset)

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
