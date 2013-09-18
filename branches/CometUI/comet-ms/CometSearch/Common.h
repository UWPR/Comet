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


#include <pthread.h>

#include <cmath>
#include <string>
#include <ctime>

#ifdef _WIN32
#include <direct.h>
#include <errno.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#endif

using namespace std;

#include "MSReader.h"
#include "Spectrum.h"
#include "MSObject.h"
#include <vector>

#ifdef CRUX
#include <iostream>
#endif

#define comet_version   "2013.02 rev. 0"
#define copyright "(c) University of Washington"

// Redefined how the bin offset is interpreted and applied.  The valid range for the offset is
// now between 0.0 and 1.0 and scales to the binWidth.
#define BIN(dMass) (int)(dMass*g_staticParams.dInverseBinWidth + g_staticParams.dOneMinusBinOffset)

#define isEqual(x, y) (std::abs(x-y) <= 1e-6 * std::abs(x))

using namespace MSToolkit;
#ifdef CRUX
#define logout(...) {char sbuf[1000]; snprintf(sbuf, 1000, __VA_ARGS__); cerr << sbuf;}
#define logerr(...) {char sbuf[1000]; snprintf(sbuf, 1000, __VA_ARGS__); cerr << sbuf;}
#else
#define logout(...) printf(__VA_ARGS__)
#define logerr(...) fprintf(stderr, __VA_ARGS__)
#endif

#endif // _COMMON_H_
