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

///////////////////////////////////////////////////////////////////////////////
//  Implementations for generic mass spectrometry related utility functions.
///////////////////////////////////////////////////////////////////////////////

#include "Common.h"
#include "CometDataInternal.h"
#include "CometSearchManager.h"
#include "CometMassSpecUtils.h"

double CometMassSpecUtils::GetFragmentIonMass(int iWhichIonSeries,
                                              int i,
                                              int ctCharge,
                                              double *pdAAforward,
                                              double *pdAAreverse)
{
   double dFragmentIonMass = 0.0;

   switch (iWhichIonSeries)
   {
      case ION_SERIES_B:
         dFragmentIonMass = pdAAforward[i];
         break;

      case ION_SERIES_Y:
         dFragmentIonMass = pdAAreverse[i];
         break;

      case ION_SERIES_A:
         dFragmentIonMass = pdAAforward[i] - g_staticParams.massUtility.dCO;
         break;

      case ION_SERIES_C:
         dFragmentIonMass = pdAAforward[i] + g_staticParams.massUtility.dNH3;
         break;

      case ION_SERIES_X:
         dFragmentIonMass = pdAAreverse[i] + g_staticParams.massUtility.dCOminusH2;
         break;

      case ION_SERIES_Z:
         dFragmentIonMass = pdAAreverse[i] - g_staticParams.massUtility.dNH2;
         break;
   }

   return (dFragmentIonMass + (ctCharge-1)*PROTON_MASS)/ctCharge;
}


void CometMassSpecUtils::AssignMass(double *pdAAMass,
                                    int bMonoMasses,
                                    double *dOH2)
{
   double H, O, C, N, S, Se;

   if (bMonoMasses) // monoisotopic masses
   {
      H = pdAAMass['h'] =  1.007825035; // hydrogen
      O = pdAAMass['o'] = 15.99491463;  // oxygen
      C = pdAAMass['c'] = 12.0000000;   // carbon
      N = pdAAMass['n'] = 14.0030740;   // nitrogen
//    P = pdAAMass['p'] = 30.973762;    // phosphorus
      S = pdAAMass['s'] = 31.9720707;   // sulphur
      Se = pdAAMass['e'] = 79.9165196;  // selenium
   }
   else  // average masses
   {
      H = pdAAMass['h'] =  1.00794;
      O = pdAAMass['o'] = 15.9994;
      C = pdAAMass['c'] = 12.0107;
      N = pdAAMass['n'] = 14.0067;
//    P = pdAAMass['p'] = 30.973761;
      S = pdAAMass['s'] = 32.065;
      Se = pdAAMass['e'] = 78.96;
   }

   *dOH2 = H + H + O;

   pdAAMass['G'] = C*2  + H*3  + N   + O ;
   pdAAMass['A'] = C*3  + H*5  + N   + O ;
   pdAAMass['S'] = C*3  + H*5  + N   + O*2 ;
   pdAAMass['P'] = C*5  + H*7  + N   + O ;
   pdAAMass['V'] = C*5  + H*9  + N   + O ;
   pdAAMass['T'] = C*4  + H*7  + N   + O*2 ;
   pdAAMass['C'] = C*3  + H*5  + N   + O   + S ;
   pdAAMass['U'] = C*3  + H*5  + N   + O   + Se ;
   pdAAMass['L'] = C*6  + H*11 + N   + O ;
   pdAAMass['I'] = C*6  + H*11 + N   + O ;
   pdAAMass['N'] = C*4  + H*6  + N*2 + O*2 ;
   pdAAMass['D'] = C*4  + H*5  + N   + O*3 ;
   pdAAMass['Q'] = C*5  + H*8  + N*2 + O*2 ;
   pdAAMass['K'] = C*6  + H*12 + N*2 + O ;
   pdAAMass['E'] = C*5  + H*7  + N   + O*3 ;
   pdAAMass['M'] = C*5  + H*9  + N   + O   + S ;
   pdAAMass['H'] = C*6  + H*7  + N*3 + O ;
   pdAAMass['F'] = C*9  + H*9  + N   + O ;
   pdAAMass['R'] = C*6  + H*12 + N*4 + O ;
   pdAAMass['Y'] = C*9  + H*9  + N   + O*2 ;
   pdAAMass['W'] = C*11 + H*10 + N*2 + O ;

   pdAAMass['O'] = C*5  + H*12 + N*2 + O*2 ;

   pdAAMass['B'] = 0.0;
   pdAAMass['J'] = 0.0;
   pdAAMass['X'] = 0.0;
   pdAAMass['Z'] = 0.0;
}
