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


#ifndef _COMBINATORICSUTILS_H_
#define _COMBINATORICSUTILS_H_

class CombinatoricsUtils
{
public:
   CombinatoricsUtils();
   ~CombinatoricsUtils();

   static int** makeCombinations(int n, int r, int count);
   static int nChooseK(int n, int k);
   static int getCombinationCount(int n, int k);
   static void initBinomialCoefficients(const int n, const int k);
};

#endif // _COMBINATORICSUTILS_H_
