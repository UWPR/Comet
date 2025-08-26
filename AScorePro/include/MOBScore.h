#pragma once

#ifndef _MOBSCORE_H_
#define _MOBSCORE_H_

#include <unordered_map>
#include <vector>
#include "AScoreOptions.h"
#include "Centroid.h"
#include "Peptide.h"
#include "Scan.h"

// Forward declarations
namespace AScoreProCpp
{
   class AScoreOutput; // Forward declaration
}

namespace AScoreProCpp
{

class MOBScore
{
private:
      class MOBPeakMatch
      {
      public:
         double error;
         int theoIndex;
         int obsIndex;
      };

      AScoreOptions options_;

      // Helper methods
      double binomialCDFUpper(int trials, int successes, double p);
      std::unordered_map<int, int> matchPeaks(const std::vector<Centroid>& theo, const std::vector<Centroid>& obs);
      double calculateScore(int ionsTotal, int ionsMatched, const std::vector<int>& matchesByPeakDepth, AScoreOutput& output);

public:
      explicit MOBScore(const AScoreOptions& options);

      double score(Peptide& peptide, const std::vector<Centroid>& ions, const Scan& scan, AScoreOutput& output);
   };

} // namespace AScoreProCpp

#endif // _MOBSCORE_H_
