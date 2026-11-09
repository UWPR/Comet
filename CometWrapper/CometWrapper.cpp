/*
   Copyright 2015 University of Washington

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

// This is the main DLL file.

#pragma region Includes

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include <msclr/marshal_cppstd.h>

#include "CometWrapper.h"

// Forward declaration: avoids including CometMassSpecUtils.h (which drags in
// CometDataInternal.h) into this C++/CLI translation unit, where
// "using namespace System" makes Threading ambiguous.
std::string GetPeakMemoryStr();

using namespace CometWrapper;

using namespace System::Runtime::InteropServices;

#pragma endregion


CometSearchManagerWrapper::CometSearchManagerWrapper()
{
    // Instantiate the native C++ class
    _pSearchMgr = GetCometSearchManager();
}

CometSearchManagerWrapper::~CometSearchManagerWrapper()
{
    // Destructor (Dispose()): deterministic cleanup path when the C# host calls
    // Dispose() or uses a `using` block. This is deliberately the ONLY place the
    // native manager is released: GetCometSearchManager()/ReleaseCometSearchManager()
    // hand every wrapper the same process-wide singleton, refcounted so the singleton
    // itself is only actually deleted once the last live wrapper releases it --
    // releasing it from a GC finalizer instead destroyed the shared native manager at
    // a nondeterministic time regardless, under any other live wrapper, or even
    // mid-native-call on this one. An undisposed wrapper leaks the singleton for the
    // life of the process, which is the safe failure mode (see the header comment).
    ReleaseCometSearchManager();
    _pSearchMgr = NULL;
}

bool CometSearchManagerWrapper::CreatePeptideIndex()
{
   if (!_pSearchMgr)
   {
      return false;
   }
   return _pSearchMgr->CreatePeptideIndex();
}

bool CometSearchManagerWrapper::CreateFragmentIndex()
{
   if (!_pSearchMgr)
   {
      return false;
   }
   return _pSearchMgr->CreateFragmentIndex();
}

bool CometSearchManagerWrapper::InitializeSingleSpectrumSearch()
{
    if (!_pSearchMgr)
    {
        return false;
    }
    return _pSearchMgr->InitializeSingleSpectrumSearch();
}

void CometSearchManagerWrapper::FinalizeSingleSpectrumSearch()
{
    if (_pSearchMgr)
    {
        _pSearchMgr->FinalizeSingleSpectrumSearch();
    }
}

bool CometSearchManagerWrapper::InitializeSingleSpectrumMS1Search(double dMaxQueryRT)
{
   if (!_pSearchMgr)
   {
      return false;
   }
   return _pSearchMgr->InitializeSingleSpectrumMS1Search(dMaxQueryRT);
}

void CometSearchManagerWrapper::FinalizeSingleSpectrumMS1Search()
{
   if (_pSearchMgr)
   {
      _pSearchMgr->FinalizeSingleSpectrumMS1Search();
   }
}

bool CometSearchManagerWrapper::DoSearch()
{
    if (!_pSearchMgr)
    {
        return false;
    }

    return _pSearchMgr->DoSearch();
}


// -----------------------------------------------------------------
// Output conversion, shared by the single-spectrum search entry points
// -----------------------------------------------------------------
// Optimization principles:
//   1. Avoid double string conversions.
//   2. Reuse managed lists if passed preallocated (optional extension).
//   3. Minimize managed object creation by reserving capacity.
// -----------------------------------------------------------------
static void ConvertSingleSpectrumResults(const std::vector<std::string>& stdPeptides,
                                         const std::vector<std::string>& stdProteins,
                                         const std::vector<std::vector<Fragment>>& stdMatchedFrags,
                                         const std::vector<CometScores>& stdScores,
                                         List<String^>^% szPeptide,
                                         List<String^>^% szProtein,
                                         List<List<FragmentWrapper^>^>^% matchingFragments,
                                         List<ScoreWrapper^>^% score)
{
   // Convert peptides/proteins (1 allocation per string)
   szPeptide = gcnew List<String^>(static_cast<int>(stdPeptides.size()));
   for (const auto& pep : stdPeptides)
      szPeptide->Add(gcnew String(pep.c_str())); // ✅ direct conversion

   szProtein = gcnew List<String^>(static_cast<int>(stdProteins.size()));
   for (const auto& prot : stdProteins)
      szProtein->Add(gcnew String(prot.c_str()));

   // Scores — allocate list with capacity and reuse wrappers if desired
   score = gcnew List<ScoreWrapper^>(static_cast<int>(stdScores.size()));
   for (const auto& s : stdScores)
      score->Add(gcnew ScoreWrapper(s));

   // Matched fragments — reserve capacity to avoid resizing overhead
   matchingFragments = gcnew List<List<FragmentWrapper^>^>(
      static_cast<int>(stdMatchedFrags.size()));

   for (const auto& fragSet : stdMatchedFrags)
   {
      auto fragList = gcnew List<FragmentWrapper^>(static_cast<int>(fragSet.size()));
      for (const auto& frag : fragSet)
         fragList->Add(gcnew FragmentWrapper(frag));

      matchingFragments->Add(fragList);
   }
}


// Input validation and native-exception containment for the RTS search calls (2026-11-08,
// branch code review item 11). Every call pins &pdMass[0] / &pdInten[0] and hands native code
// iNumPeaks, which it reads that many elements of: a null array, an empty array (&a[0] throws
// IndexOutOfRangeException) or an iNumPeaks larger than either array (a reused buffer with a
// stale count, or mass/intensity arrays of different lengths -- a read past the pinned managed
// array) is rejected here with false and empty outputs. A longer array than iNumPeaks is fine:
// hosts reuse buffers. A native C++ exception (std::bad_alloc, ...) escaping a call would reach
// the host as an SEHException and can end a real-time acquisition loop, so it is caught and the
// call returns false; GetLastWrapperError() says why.
static bool ValidPeakArrays(cli::array<double>^ pdMass, cli::array<double>^ pdInten, int iNumPeaks)
{
   return pdMass != nullptr && pdInten != nullptr && iNumPeaks > 0
      && pdMass->Length >= iNumPeaks && pdInten->Length >= iNumPeaks;
}

static String^ DescribePeakArrays(cli::array<double>^ pdMass, cli::array<double>^ pdInten, int iNumPeaks)
{
   return String::Format("invalid peak arrays: mass {0}, intensity {1}, iNumPeaks {2}",
      pdMass == nullptr ? "null" : pdMass->Length.ToString(),
      pdInten == nullptr ? "null" : pdInten->Length.ToString(), iNumPeaks);
}

static void EmptySingleSpectrumOutputs(List<String^>^% szPeptide,
   List<String^>^% szProtein,
   List<List<FragmentWrapper^>^>^% matchingFragments,
   List<ScoreWrapper^>^% score)
{
   szPeptide = gcnew List<String^>();
   szProtein = gcnew List<String^>();
   matchingFragments = gcnew List<List<FragmentWrapper^>^>();
   score = gcnew List<ScoreWrapper^>();
}


// The native calls of the two chimeric overloads, as function objects over the pinned peak
// arrays (a lambda is not allowed inside a member function of a managed class, C3923).
struct ChimericListCall
{
   ICometSearchManager* pMgr;
   const std::vector<PrecursorCandidate>* pvPrecursors;
   double dPrecursorToleranceMZ;
   double dMaxExpect;
   int iMaxPeptidesPerSpectrum;
   double* pdMass;
   double* pdInten;
   int iNumPeaks;

   bool operator()(std::vector<std::string>& vPep, std::vector<std::string>& vProt,
                   std::vector<std::vector<Fragment>>& vFrag, std::vector<CometScores>& vScores,
                   bool& bNativeIncomplete) const
   {
      return pMgr->DoSingleSpectrumSearchChimeric(*pvPrecursors, dPrecursorToleranceMZ, dMaxExpect,
         iMaxPeptidesPerSpectrum, pdMass, pdInten, iNumPeaks, vPep, vProt, vFrag, vScores, bNativeIncomplete);
   }
};

struct ChimericRangeCall
{
   ICometSearchManager* pMgr;
   double dMZ;
   int iMinPrecursorCharge;
   int iMaxPrecursorCharge;
   double dPrecursorToleranceMZ;
   double dMaxExpect;
   int iMaxPeptidesPerSpectrum;
   double* pdMass;
   double* pdInten;
   int iNumPeaks;

   bool operator()(std::vector<std::string>& vPep, std::vector<std::string>& vProt,
                   std::vector<std::vector<Fragment>>& vFrag, std::vector<CometScores>& vScores,
                   bool& bNativeIncomplete) const
   {
      return pMgr->DoSingleSpectrumSearchChimeric(dMZ, iMinPrecursorCharge, iMaxPrecursorCharge,
         dPrecursorToleranceMZ, dMaxExpect, iMaxPeptidesPerSpectrum, pdMass, pdInten, iNumPeaks,
         vPep, vProt, vFrag, vScores, bNativeIncomplete);
   }
};

// The shared tail of both chimeric overloads: run the native search (fnSearch, one of the
// function objects above), contain a native exception, and convert the rows. The rows
// appended before an exception are complete iterations, so they are kept, as a cancelled call
// keeps them (bIncomplete true, return false), unless an exception mid-row left the four
// vectors misaligned. strError (nullptr on entry) receives the exception text; the caller
// stores it for GetLastWrapperError().
template <typename FnSearch>
static bool RunChimericSearch(const FnSearch& fnSearch,
   String^% strError,
   List<String^>^% szPeptide,
   List<String^>^% szProtein,
   List<List<FragmentWrapper^>^>^% matchingFragments,
   List<ScoreWrapper^>^% score,
   bool% bIncomplete)
{
   std::vector<std::string> stdPeptides;
   std::vector<std::string> stdProteins;
   std::vector<CometScores> stdScores;
   std::vector<std::vector<Fragment>> stdMatchedFrags;

   bool ok = false;
   bool bNativeIncomplete = false;
   try
   {
      ok = fnSearch(stdPeptides, stdProteins, stdMatchedFrags, stdScores, bNativeIncomplete);
   }
   catch (const std::exception& e)
   {
      strError = "native exception: " + gcnew String(e.what());
   }
   catch (...)
   {
      strError = "unknown native exception";
   }
   if (strError != nullptr)
   {
      bIncomplete = true;
      const size_t n = stdScores.size();
      if (stdPeptides.size() == n && stdProteins.size() == n && stdMatchedFrags.size() == n)
         ConvertSingleSpectrumResults(stdPeptides, stdProteins, stdMatchedFrags, stdScores,
            szPeptide, szProtein, matchingFragments, score);
      else
         EmptySingleSpectrumOutputs(szPeptide, szProtein, matchingFragments, score);
      return false;
   }
   bIncomplete = bNativeIncomplete;

   ConvertSingleSpectrumResults(stdPeptides, stdProteins, stdMatchedFrags, stdScores,
      szPeptide, szProtein, matchingFragments, score);
   return ok;
}


String^ CometSearchManagerWrapper::GetLastWrapperError()
{
   return s_strLastWrapperError;
}


bool CometSearchManagerWrapper::DoSingleSpectrumSearchMultiResults(
   int topN,
   int iPrecursorCharge,
   double dMZ,
   cli::array<double>^ pdMass,
   cli::array<double>^ pdInten,
   int iNumPeaks,
   [Out] List<String^>^% szPeptide,
   [Out] List<String^>^% szProtein,
   [Out] List<List<FragmentWrapper^>^>^% matchingFragments,
   [Out] List<ScoreWrapper^>^% score)
{
   s_strLastWrapperError = nullptr;
   if (!_pSearchMgr || !ValidPeakArrays(pdMass, pdInten, iNumPeaks))
   {
      s_strLastWrapperError = !_pSearchMgr ? "no search manager" : DescribePeakArrays(pdMass, pdInten, iNumPeaks);
      EmptySingleSpectrumOutputs(szPeptide, szProtein, matchingFragments, score);
      return false;
   }

   // --- Input pinning (no copies, already optimal) ---
   pin_ptr<double> ptrMasses = &pdMass[0];
   pin_ptr<double> ptrInten = &pdInten[0];

   // --- Prepare native output containers ---
   std::vector<std::string> stdPeptides;
   std::vector<std::string> stdProteins;
   std::vector<CometScores> stdScores;
   std::vector<std::vector<Fragment>> stdMatchedFrags;

   // --- Perform native search ---
   bool ok = false;
   try
   {
      ok = _pSearchMgr->DoSingleSpectrumSearchMultiResults(
         topN, iPrecursorCharge, dMZ,
         ptrMasses, ptrInten, iNumPeaks,
         stdPeptides, stdProteins, stdMatchedFrags, stdScores);
   }
   catch (const std::exception& e)
   {
      s_strLastWrapperError = "native exception: " + gcnew String(e.what());
      ok = false;
   }
   catch (...)
   {
      s_strLastWrapperError = "unknown native exception";
      ok = false;
   }
   if (s_strLastWrapperError != nullptr)
   {
      EmptySingleSpectrumOutputs(szPeptide, szProtein, matchingFragments, score);
      return false;
   }

   ConvertSingleSpectrumResults(stdPeptides, stdProteins, stdMatchedFrags, stdScores,
      szPeptide, szProtein, matchingFragments, score);

   return ok;
}


bool CometSearchManagerWrapper::DoSingleSpectrumSearchChimeric(
   cli::array<double>^ pdCandidateMZ,
   cli::array<int>^ piCandidateCharge,
   cli::array<double>^ pdCandidateToleranceMZ,
   double dPrecursorToleranceMZ,
   double dMaxExpect,
   int iMaxPeptidesPerSpectrum,
   cli::array<double>^ pdMass,
   cli::array<double>^ pdInten,
   const int iNumPeaks,
   [Out] List<String^>^% szPeptide,
   [Out] List<String^>^% szProtein,
   [Out] List<List<FragmentWrapper^>^>^% matchingFragments,
   [Out] List<ScoreWrapper^>^% score,
   [Out] bool% bIncomplete)
{
   bIncomplete = false;
   s_strLastWrapperError = nullptr;
   if (!_pSearchMgr || pdCandidateMZ == nullptr || piCandidateCharge == nullptr
      || pdCandidateMZ->Length == 0 || piCandidateCharge->Length != pdCandidateMZ->Length
      || (pdCandidateToleranceMZ != nullptr && pdCandidateToleranceMZ->Length != pdCandidateMZ->Length)
      || !ValidPeakArrays(pdMass, pdInten, iNumPeaks))
   {
      if (!_pSearchMgr)
         s_strLastWrapperError = "no search manager";
      else if (!ValidPeakArrays(pdMass, pdInten, iNumPeaks))
         s_strLastWrapperError = DescribePeakArrays(pdMass, pdInten, iNumPeaks);
      else
         s_strLastWrapperError = "invalid candidate arrays: m/z, charge and tolerance arrays must be non-empty and of equal length";
      EmptySingleSpectrumOutputs(szPeptide, szProtein, matchingFragments, score);
      return false;
   }

   // The native candidate list is the only per-candidate allocation; the peak arrays are pinned.
   std::vector<PrecursorCandidate> vPrecursors;
   vPrecursors.reserve(pdCandidateMZ->Length);
   for (int i = 0; i < pdCandidateMZ->Length; ++i)
   {
      // Copy out first: a managed array element cannot bind to the native references
      // emplace_back's forwarding would deduce (C2664).
      const double dCandMZ = pdCandidateMZ[i];
      const int iCandCharge = piCandidateCharge[i];
      const double dCandTol = (pdCandidateToleranceMZ != nullptr) ? pdCandidateToleranceMZ[i] : 0.0;
      vPrecursors.push_back(PrecursorCandidate(dCandMZ, iCandCharge, dCandTol));
   }

   pin_ptr<double> ptrMasses = &pdMass[0];
   pin_ptr<double> ptrInten = &pdInten[0];
   ChimericListCall call = { _pSearchMgr, &vPrecursors, dPrecursorToleranceMZ, dMaxExpect,
                             iMaxPeptidesPerSpectrum, ptrMasses, ptrInten, iNumPeaks };

   String^ strError = nullptr;
   const bool ok = RunChimericSearch(call,
      strError, szPeptide, szProtein, matchingFragments, score, bIncomplete);
   s_strLastWrapperError = strError;
   return ok;
}


bool CometSearchManagerWrapper::DoSingleSpectrumSearchChimeric(
   double dMZ,
   int iMinPrecursorCharge,
   int iMaxPrecursorCharge,
   double dPrecursorToleranceMZ,
   double dMaxExpect,
   int iMaxPeptidesPerSpectrum,
   cli::array<double>^ pdMass,
   cli::array<double>^ pdInten,
   const int iNumPeaks,
   [Out] List<String^>^% szPeptide,
   [Out] List<String^>^% szProtein,
   [Out] List<List<FragmentWrapper^>^>^% matchingFragments,
   [Out] List<ScoreWrapper^>^% score,
   [Out] bool% bIncomplete)
{
   bIncomplete = false;
   s_strLastWrapperError = nullptr;
   if (!_pSearchMgr || !ValidPeakArrays(pdMass, pdInten, iNumPeaks))
   {
      s_strLastWrapperError = !_pSearchMgr ? "no search manager" : DescribePeakArrays(pdMass, pdInten, iNumPeaks);
      EmptySingleSpectrumOutputs(szPeptide, szProtein, matchingFragments, score);
      return false;
   }

   pin_ptr<double> ptrMasses = &pdMass[0];
   pin_ptr<double> ptrInten = &pdInten[0];
   ChimericRangeCall call = { _pSearchMgr, dMZ, iMinPrecursorCharge, iMaxPrecursorCharge,
                              dPrecursorToleranceMZ, dMaxExpect, iMaxPeptidesPerSpectrum, ptrMasses, ptrInten,
                              iNumPeaks };

   String^ strError = nullptr;
   const bool ok = RunChimericSearch(call,
      strError, szPeptide, szProtein, matchingFragments, score, bIncomplete);
   s_strLastWrapperError = strError;
   return ok;
}


bool CometSearchManagerWrapper::DoMS1SearchMultiResults(double dMaxMS1RTDiff,
   double dMaxQueryRT,
   int topN,
   double dRT,
   cli::array<double>^ pdMass,
   cli::array<double>^ pdInten,
   int iNumPeaks,
   [Out] List<ScoreWrapperMS1^>^% scoreMS1)
{
   s_strLastWrapperError = nullptr;
   scoreMS1 = gcnew List<ScoreWrapperMS1^>();
   if (!_pSearchMgr || !ValidPeakArrays(pdMass, pdInten, iNumPeaks))
   {
      s_strLastWrapperError = !_pSearchMgr ? "no search manager" : DescribePeakArrays(pdMass, pdInten, iNumPeaks);
      return false;
   }
   pin_ptr<double> ptrMasses = &pdMass[0];
   pin_ptr<double> ptrInten = &pdInten[0];

   vector<CometScoresMS1> scoresMS1;

   // perform the search
   bool isSuccess = false;
   try
   {
      isSuccess = _pSearchMgr->DoMS1SearchMultiResults(dMaxMS1RTDiff, dMaxQueryRT, topN, dRT, ptrMasses, ptrInten, iNumPeaks, scoresMS1);
   }
   catch (const std::exception& e)
   {
      s_strLastWrapperError = "native exception: " + gcnew String(e.what());
   }
   catch (...)
   {
      s_strLastWrapperError = "unknown native exception";
   }
   if (s_strLastWrapperError != nullptr)
      return false;

   for (auto eachScore : scoresMS1)
   {
      scoreMS1->Add(gcnew ScoreWrapperMS1(eachScore));
   }

   return isSuccess;
}

bool CometSearchManagerWrapper::AddInputFiles(List<InputFileInfoWrapper^> ^inputFilesList)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    // Hand the native side its own deep copies, and only THIS call's files:
    // ~CometSearchManager() deletes every InputFileInfo* in g_pvInputFiles, while each
    // InputFileInfoWrapper also deletes the InputFileInfo it owns -- passing the
    // wrapper-owned pointer through would double-free. Copying also fixes the old
    // accumulated-list bug where a second AddInputFiles call re-added the first
    // batch's pointers to g_pvInputFiles (duplicate entries, double delete).
    vector<InputFileInfo*> vNewInputFiles;
    int numFiles = inputFilesList->Count;
    try
    {
        for (int i = 0; i < numFiles; i++)
        {
            InputFileInfoWrapper^ inputFile = inputFilesList[i];
            vNewInputFiles.push_back(new InputFileInfo(*(inputFile->get_InputFileInfoPtr())));
        }
    }
    catch (...)
    {
        for (auto pFile : vNewInputFiles)
            delete pFile;
        throw;
    }

    _pSearchMgr->AddInputFiles(vNewInputFiles);

    return true;
}

bool CometSearchManagerWrapper::SetOutputFileBaseName(System::String^ baseName)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    const char* pszBaseName = _marshalContext.marshal_as<const char*>(baseName);
    _pSearchMgr->SetOutputFileBaseName(pszBaseName);

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, System::String^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }
    
    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue); 
    std::string stdStringValue = marshal_as<std::string>(value);  

    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, stdStringValue);
    
    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, System::String^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    std::string stdStringValue;
    if (!_pSearchMgr->GetParamValue(stdStringName, stdStringValue))
    {
        return false;
    }
    
    value = gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(stdStringValue.c_str())))); 

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, int value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue); 
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, value);
    
    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, int %value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    int iValue;
    if (!_pSearchMgr->GetParamValue(stdStringName, iValue))
    {
        return false;
    }
    

    value = iValue; 

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, double value)
{
    if (!_pSearchMgr)
    {
        return false;
    }
    
    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue); 
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, value);
    
    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, double% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    double dValue;
    if (!_pSearchMgr->GetParamValue(stdStringName, dValue))
    {
        return false;
    }

    value = dValue; 

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, IntRangeWrapper^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    IntRange *pIntRange = value->get_IntRangePtr();
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, *pIntRange);

    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, IntRangeWrapper^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    IntRange intRangeParam(0, 0);
    if (!_pSearchMgr->GetParamValue(stdStringName, intRangeParam))
    {
        return false;
    }

    value = gcnew IntRangeWrapper(intRangeParam);

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, DoubleRangeWrapper^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    DoubleRange *pDoubleRange = value->get_DoubleRangePtr();
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, *pDoubleRange);

    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, DoubleRangeWrapper^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    DoubleRange doubleRangeParam(0.0, 0.0);
    if (!_pSearchMgr->GetParamValue(stdStringName, doubleRangeParam))
    {
        return false;
    }

    value = gcnew DoubleRangeWrapper(doubleRangeParam);

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, VarModsWrapper^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    VarMods *pVarMods = value->get_VarModsPtr();
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, *pVarMods);

    return true;
}

bool CometSearchManagerWrapper::GetParamValue(System::String^ name, VarModsWrapper^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    VarMods varModsParam;
    if (!_pSearchMgr->GetParamValue(stdStringName, varModsParam))
    {
        return false;
    }

    value = gcnew VarModsWrapper(varModsParam);

    return true;
}

bool CometSearchManagerWrapper::SetParam(System::String^ name, System::String^ strValue, EnzymeInfoWrapper^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    EnzymeInfo *pEnzymInfo = value->get_EnzymeInfoPtr();
    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, *pEnzymInfo);

    return true;
}


bool CometSearchManagerWrapper::GetParamValue(System::String^ name, EnzymeInfoWrapper^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    EnzymeInfo enzymeInfoParam;
    if (!_pSearchMgr->GetParamValue(stdStringName, enzymeInfoParam))
    {
        return false;
    }

    value = gcnew EnzymeInfoWrapper(enzymeInfoParam);

    return true;
}

bool CometSearchManagerWrapper::SetParam(String^ name, String^ strValue, List<double>^ value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name); 
    std::string stdStringStrValue = marshal_as<std::string>(strValue);
    
    vector<double> vectorMassOffsets;
    int numItems = value->Count;
    for (int i = 0; i < numItems; i++)
    {
        vectorMassOffsets.push_back(value[i]);
    }
    sort(vectorMassOffsets.begin(), vectorMassOffsets.end());

    _pSearchMgr->SetParam(stdStringName, stdStringStrValue, vectorMassOffsets);

    return true;
}

bool CometSearchManagerWrapper::GetParamValue(String^ name, List<double>^% value)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStringName = marshal_as<std::string>(name);
    vector<double> vectorMassOffsets;
    if (!_pSearchMgr->GetParamValue(stdStringName, vectorMassOffsets))
    {
        return false;
    }

    int numItems = (int)vectorMassOffsets.size();
    for (int i = 0; i < numItems; i++)
    {
        value->Add(vectorMassOffsets[i]);
    }

    return true;
}

bool CometSearchManagerWrapper::ValidateCometVersion(String^ version, bool% isValid)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdVersion = marshal_as<std::string>(version);
    isValid = _pSearchMgr->IsValidCometVersion(stdVersion);
    return true;
}

bool CometSearchManagerWrapper::IsSearchError(bool% bError)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    bError = _pSearchMgr->IsSearchError();
    return true;
}

bool CometSearchManagerWrapper::GetStatusMessage(System::String^% strStatusMsg)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    std::string stdStrStatusMsg;
    _pSearchMgr->GetStatusMessage(stdStrStatusMsg);
    strStatusMsg = gcnew String(Marshal::PtrToStringAnsi(static_cast<IntPtr>(const_cast<char *>(stdStrStatusMsg.c_str())))); 

    return true;
}

bool CometSearchManagerWrapper::CancelSearch()
{
    if (!_pSearchMgr)
    {
        return false;
    }

    _pSearchMgr->CancelSearch();
    return true;
}

bool CometSearchManagerWrapper::IsCancelSearch(bool% bCancel)
{
    if (!_pSearchMgr)
    {
        return false;
    }

    bCancel = _pSearchMgr->IsCancelSearch();
    return true;
}

bool CometSearchManagerWrapper::ResetSearchStatus()
{
    if (!_pSearchMgr)
    {
        return false;
    }

    _pSearchMgr->ResetSearchStatus();
    return true;
}

String^ CometSearchManagerWrapper::GetPeakMemory()
{
   std::string s = GetPeakMemoryStr();
   return gcnew String(s.c_str());
}
