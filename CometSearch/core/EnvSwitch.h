// Copyright 2012-2026 Jimmy Eng
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

#ifndef _COMETENVSWITCH_H_
#define _COMETENVSWITCH_H_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// A testing switch read from an environment variable (COMET_PREPROCESS, COMET_DECOYGEN): not a
// parameter. The value names one of pszNames; unset or empty means the first name (the default,
// "auto"), and an unrecognized value is reported on stderr and also means the first name. Returns
// the index of the chosen name, so a caller keeps its enum in the order of its list of names.
// Callers read it once per process into a function-local static.
inline int ReadEnvSwitch(const char* szEnvName, const char* const* pszNames, int iNumNames)
{
   const char* psz = getenv(szEnvName);
   if (psz == nullptr || *psz == '\0')
      return 0;

   for (int i = 0; i < iNumNames; ++i)
   {
      if (strcmp(psz, pszNames[i]) == 0)
         return i;
   }

   std::string strList;
   for (int i = 0; i < iNumNames; ++i)
      strList += (i > 0 ? ", " : "") + std::string(pszNames[i]);
   fprintf(stderr, " %s=%s not recognized (%s); using %s\n", szEnvName, psz, strList.c_str(), pszNames[0]);
   return 0;
}

#endif
