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

      case ION_SERIES_Z1:
         dFragmentIonMass = pdAAreverse[i] - g_staticParams.massUtility.dNH2 + Hydrogen_Mono;
         break;
   }

   return (dFragmentIonMass + (ctCharge - 1.0) * PROTON_MASS) / ctCharge;
}


void CometMassSpecUtils::AssignMass(double *pdAAMass,
                                    int bMonoMasses,
                                    double *dOH2)
{
   double H, O, C, N, S, Se;

   if (bMonoMasses) // monoisotopic masses
   {
      H = pdAAMass[(int)'h'] = Hydrogen_Mono; // hydrogen
      O = pdAAMass[(int)'o'] = Oxygen_Mono;  // oxygen
      C = pdAAMass[(int)'c'] = Carbon_Mono;   // carbon
      N = pdAAMass[(int)'n'] = Nitrogen_Mono;   // nitrogen
//    P = pdAAMass[(int)'p'] = 30.973762;    // phosphorus
      S = pdAAMass[(int)'s'] = 31.9720707;   // sulphur
      Se = pdAAMass[(int)'e'] = 79.9165196;  // selenium
   }
   else  // average masses
   {
      H = pdAAMass[(int)'h'] =  1.00794;
      O = pdAAMass[(int)'o'] = 15.9994;
      C = pdAAMass[(int)'c'] = 12.0107;
      N = pdAAMass[(int)'n'] = 14.0067;
//    P = pdAAMass[(int)'p'] = 30.973761;
      S = pdAAMass[(int)'s'] = 32.065;
      Se = pdAAMass[(int)'e'] = 78.96;
   }

   *dOH2 = H + H + O;

   pdAAMass[(int)'G'] = C*2  + H*3  + N   + O ;
   pdAAMass[(int)'A'] = C*3  + H*5  + N   + O ;
   pdAAMass[(int)'S'] = C*3  + H*5  + N   + O*2 ;
   pdAAMass[(int)'P'] = C*5  + H*7  + N   + O ;
   pdAAMass[(int)'V'] = C*5  + H*9  + N   + O ;
   pdAAMass[(int)'T'] = C*4  + H*7  + N   + O*2 ;
   pdAAMass[(int)'C'] = C*3  + H*5  + N   + O   + S ;
   pdAAMass[(int)'U'] = C*3  + H*5  + N   + O   + Se ;
   pdAAMass[(int)'L'] = C*6  + H*11 + N   + O ;
   pdAAMass[(int)'I'] = C*6  + H*11 + N   + O ;
   pdAAMass[(int)'N'] = C*4  + H*6  + N*2 + O*2 ;
   pdAAMass[(int)'D'] = C*4  + H*5  + N   + O*3 ;
   pdAAMass[(int)'Q'] = C*5  + H*8  + N*2 + O*2 ;
   pdAAMass[(int)'K'] = C*6  + H*12 + N*2 + O ;
   pdAAMass[(int)'E'] = C*5  + H*7  + N   + O*3 ;
   pdAAMass[(int)'M'] = C*5  + H*9  + N   + O   + S ;
   pdAAMass[(int)'H'] = C*6  + H*7  + N*3 + O ;
   pdAAMass[(int)'F'] = C*9  + H*9  + N   + O ;
   pdAAMass[(int)'R'] = C*6  + H*12 + N*4 + O ;
   pdAAMass[(int)'Y'] = C*9  + H*9  + N   + O*2 ;
   pdAAMass[(int)'W'] = C*11 + H*10 + N*2 + O ;

   pdAAMass[(int)'O'] = C*12  + H*19 + N*3 + O*2 ;

   pdAAMass[(int)'B'] = 0.0;
   pdAAMass[(int)'J'] = 0.0;
   pdAAMass[(int)'X'] = 0.0;
   pdAAMass[(int)'Z'] = 0.0;
}


// return a single protein name as a C char string
void CometMassSpecUtils::GetProteinName(FILE *fpdb,
                                        comet_fileoffset_t lFilePosition,
                                        char *szProteinName)
{
   comet_fseek(fpdb, lFilePosition, SEEK_SET);

   if (g_staticParams.bIndexDb)  //index database
   {
      long lSize;

      fread(&lSize, sizeof(long), 1, fpdb);
      vector<comet_fileoffset_t> vOffsets;
      for (long x = 0; x < lSize; ++x)
      {
         comet_fileoffset_t tmpoffset;
         fread(&tmpoffset, sizeof(comet_fileoffset_t), 1, fpdb);
         vOffsets.push_back(tmpoffset);
      }
      for (long x = 0; x < lSize; ++x)
      {
         char szTmp[WIDTH_REFERENCE];
         comet_fseek(fpdb, vOffsets.at(x), SEEK_SET);
         fread(szTmp, sizeof(char)*WIDTH_REFERENCE, 1, fpdb);
         sscanf(szTmp, "%511s", szProteinName);  // WIDTH_REFERENCE-1
         break;  //break here to only get first protein reference (out of lSize)
      }
   }
   else  //regular fasta database
   {
      fscanf(fpdb, "%511s", szProteinName);  // WIDTH_REFERENCE-1
      szProteinName[511] = '\0';
   }
}


// return a single protein sequence as C++ string
void CometMassSpecUtils::GetProteinSequence(FILE *fpdb,
                                            comet_fileoffset_t lFilePosition,
                                            string &strSeq)
{
   strSeq.clear();

   if (!g_staticParams.bIndexDb)  // works only for regular FASTA
   {
      int iTmpCh;

      comet_fseek(fpdb, lFilePosition, SEEK_SET);

      // skip to end of description line
      while (((iTmpCh = getc(fpdb)) != '\n') && (iTmpCh != '\r') && (iTmpCh != EOF));

      // load sequence
      while (((iTmpCh=getc(fpdb)) != '>') && (iTmpCh != EOF))
      {
         if ('a'<=iTmpCh && iTmpCh<='z')
         {
            strSeq += iTmpCh - 32;  // convert toupper case so subtract 32 (i.e. 'A'-'a')
            g_staticParams.databaseInfo.uliTotAACount++;
         }
         else if ('A'<=iTmpCh && iTmpCh<='Z')
         {
            strSeq += iTmpCh;
            g_staticParams.databaseInfo.uliTotAACount++;
         }
         else if (iTmpCh == '*')  // stop codon
         {
            strSeq += iTmpCh;
         }
      }
   }
}


// return all matched protein names in a vector of strings
void CometMassSpecUtils::GetProteinNameString(FILE *fpdb,
                                              int iWhichQuery,  // which search
                                              int iWhichResult, // which peptide within the search
                                              int iPrintTargetDecoy,    // 0 = target+decoys, 1=target only, 2=decoy only
                                              vector<string>& vProteinTargets,  // the target protein names
                                              vector<string>& vProteinDecoys)   // the decoy protein names if applicable
{
   char szProteinName[WIDTH_REFERENCE];
   const int iDECOY_WIDTH_REFERENCE = WIDTH_REFERENCE+256;
   char szDecoyProteinName[iDECOY_WIDTH_REFERENCE];
   std::vector<ProteinEntryStruct>::iterator it;

   int iLenDecoyPrefix = strlen(g_staticParams.szDecoyPrefix);

   if (g_staticParams.bIndexDb)  //index database
   {
      Results *pOutput;

      if (iPrintTargetDecoy != 2)
         pOutput = g_pvQuery.at(iWhichQuery)->_pResults;
      else
         pOutput = g_pvQuery.at(iWhichQuery)->_pDecoys;

      // get target proteins
      if (iPrintTargetDecoy != 2)  // if not decoy-only
      {
         comet_fileoffset_t lEntry = pOutput[iWhichResult].lProteinFilePosition;
         for (auto it = g_pvProteinsList.at(lEntry).begin(); it != g_pvProteinsList.at(lEntry).end(); ++it)
         {
            comet_fseek(fpdb, *it, SEEK_SET);
            fscanf(fpdb, "%511s", szProteinName);  // WIDTH_REFERENCE-1
            szProteinName[511] = '\0';
            vProteinTargets.push_back(szProteinName);
         }
      }

      // FIX need to handle decoys

   }
   else  // regular fasta database
   {
      Results *pOutput;

      if (iPrintTargetDecoy != 2)
         pOutput = g_pvQuery.at(iWhichQuery)->_pResults;
      else
         pOutput = g_pvQuery.at(iWhichQuery)->_pDecoys;

      int iPrintDuplicateProteinCt = 0; // track # proteins, exit when at iMaxDuplicateProteins

      // targets + decoys, targets only, decoys only
 
      // get target proteins
      if (iPrintTargetDecoy != 2)  // if not decoy-only
      {
         if (pOutput[iWhichResult].pWhichProtein.size() > 0)
         {
            for (it=pOutput[iWhichResult].pWhichProtein.begin(); it!=pOutput[iWhichResult].pWhichProtein.end(); ++it)
            {
               comet_fseek(fpdb, (*it).lWhichProtein, SEEK_SET);
               fscanf(fpdb, "%511s", szProteinName);  // WIDTH_REFERENCE-1
               szProteinName[511] = '\0';

               vProteinTargets.push_back(szProteinName);
               iPrintDuplicateProteinCt++;
               if (iPrintDuplicateProteinCt > g_staticParams.options.iMaxDuplicateProteins)
                  break;
            }
         }
      }

      // get decoy proteins
      if (iPrintTargetDecoy != 1)  // if not target-only
      {
         if (pOutput[iWhichResult].pWhichDecoyProtein.size() > 0)
         {
            // collate decoy proteins, if needed, from target-decoy search
            for (it=pOutput[iWhichResult].pWhichDecoyProtein.begin(); it!=pOutput[iWhichResult].pWhichDecoyProtein.end(); ++it)
            {
               if (iPrintDuplicateProteinCt > g_staticParams.options.iMaxDuplicateProteins)
                  break;
   
               comet_fseek(fpdb, (*it).lWhichProtein, SEEK_SET);
               fscanf(fpdb, "%511s", szProteinName);  // WIDTH_REFERENCE-1
               szProteinName[511] = '\0';
   
               if (strlen(szProteinName) + iLenDecoyPrefix >= WIDTH_REFERENCE)
                  szProteinName[strlen(szProteinName) - iLenDecoyPrefix] = '\0';
               sprintf(szDecoyProteinName, "%s%s", g_staticParams.szDecoyPrefix, szProteinName);
   
               vProteinDecoys.push_back(szDecoyProteinName);
               iPrintDuplicateProteinCt++;
            }
         }
      }
   }
}


// return nth entry in string s
string CometMassSpecUtils::GetField(std::string *s,
                                    unsigned int n,
                                    char cDelimeter)
{
   std::string field;
   std::istringstream isString(*s);

   while ( std::getline(isString, field, cDelimeter) )
   {
      n--;

      if (n == 0)
         break;
   }

   return field;
}


// escape string for XML output
void CometMassSpecUtils::EscapeString(std::string& data)
{
   if (     strchr(data.c_str(), '&')
         || strchr(data.c_str(), '\"')
         || strchr(data.c_str(), '\'')
         || strchr(data.c_str(), '<')
         || strchr(data.c_str(), '>'))
   {
      std::string buffer;
      buffer.reserve(data.size());
      for(size_t pos = 0; pos != data.size(); ++pos)
      {
         switch(data[pos])
         {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(&data[pos], 1); break;
         }
      }
      data.swap(buffer);
   }
}
