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
                                    vector<string>& vProteinTargets,  // the target protein names
                                    vector<string>& vProteinDecoys);  // the decoy protein names if applicable

   static string GetField(std::string *s,
                          unsigned int n,
                          char cDelimeter);

   static void EscapeString(std::string& data);
};

#endif // _COMETMASSSPECUTILS_H_
