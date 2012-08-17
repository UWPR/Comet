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
#include <winsock2.h>
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


#define version   "201?.?? rev. ?"
#define copyright "(c) University of Washington"

// NOTE: dBinWidth is inverse of the input value in order to use multiply instead of divide here
#define BIN(dMass) ((int)((dMass + g_StaticParams.dBinWidthMinusOffset)*g_StaticParams.dBinWidth))

#endif // _COMMON_H_
