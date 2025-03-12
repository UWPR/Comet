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


///////////////////////////////////////////////////////////////////////////////
//  Definitions for generic mass spectrometry related utility functions.
///////////////////////////////////////////////////////////////////////////////

#ifndef _COMETMASSSPECUTILS_H_
#define _COMETMASSSPECUTILS_H_

const double Hydrogen_Mono = 1.007825035;
const double Oxygen_Mono = 15.99491463;
const double Carbon_Mono = 12.00000000;
const double Nitrogen_Mono = 14.0030740;

class CometMassSpecUtils
{
public:
   static double GetFragmentIonMass(int iWhichIonSeries,
                                    int i,
                                    int ctCharge,
                                    double *pdAAforward,
                                    double *pdAAreverse);

   static void AssignMass(double *pdAAMass,
                          int bMonoMasses,
                          double *dOH2);

   static void GetProteinName(FILE *fpdb,
                              comet_fileoffset_t lFilePosition,
                              char *szProteinName);

   static void GetProteinSequence(FILE *fpdb,
                                  comet_fileoffset_t lFilePosition,
                                  string &strSeq);

   static void GetProteinNameString(FILE *fpdb,
                                    int iWhichQuery,  // which search
                                    int iWhichResult, // which peptide within the search
                                    int iPrintTargetDecoy,
                                    bool bReturnFullProteinString,   // 0 = return accession only, 1 = return full description line
                                    unsigned int *iNumTotProteins,   // matched protein count
                                    vector<string>& vProteinTargets,  // the target protein names
                                    vector<string>& vProteinDecoys);  // the decoy protein names if applicable

   static void GetPrevNextAA(FILE *fpdb,
                             int iWhichQuery,  // which search
                             int iWhichResult, // which peptide within the search
                             int iPrintTargetDecoy,
                             int iWhichTerm);  // 0=no term constraint, 1=protein N-term, 2=protein C-term

   static bool SeekPrevNextAA(struct Results *pOutput,
                              FILE *fpfasta,
                              comet_fileoffset_t tFilePos,
                              int iWhichQuery,
                              int iWhichResult,
                              int iWhichTerm);

   static string GetField(std::string *s,
                          unsigned int n,
                          char cDelimeter);

   static void EscapeString(std::string& data);
};

#endif // _COMETMASSSPECUTILS_H_
