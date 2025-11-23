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


#ifndef _COMETSPECLIB_H_
#define _COMETSPECLIB_H_

#include "Common.h"
#include "CometDataInternal.h"
#include "CometSearch.h"

class CometSpecLib
{
public:
   CometSpecLib();
   ~CometSpecLib();

   static bool LoadSpecLib(std::string strSpecLibFile);
   static bool SearchSpecLib(int iWhichQuery,
                             ThreadPool *tp);
   static double ScoreSpecLib(Query *it,
                              unsigned int iWhichSpecLib);
   static void StoreSpecLib(Query *it,
                            unsigned int iWhichSpecLib,
                            double dSpecLibScore);
   static bool LoadSpecLibMS1Raw(ThreadPool* tp,
                                 const double dMaxQueryRT,
                                 double* dMaxSpecLibRT);


private:
   static bool ReadSpecLibRaw(std::string strSpecLib);
   static bool ReadSpecLibSqlite(std::string strSpecLib);
   static bool ReadSpecLibMSP(std::string strSpecLib);
   static std::vector<double> decodeBlob(const void* blob, int size);
   static void printDoubleVector(const std::vector<double>& vec);
   static void SetSpecLibPrecursorIndex(double dNeutralMass,
                                        int iCharge,
                                        size_t iWhichSpecLib);

};

#endif // _COMETSPECLIB_H_
