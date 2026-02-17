#include "AScoreMOBScore.h"
#include "AScoreOutput.h"
#include "AScoreBinomial.h"
#include "AScorePeakMatcher.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>

namespace AScoreProCpp
{

   MOBScore::MOBScore(const AScoreOptions& options) : options_(options)
   {
   }

   double MOBScore::score(Peptide& peptide, const std::vector<Centroid>& ions, const Scan& scan, AScoreOutput& output)
   {
      int maxPeakDepth = options_.getMaxPeakDepth();

      const auto& peaks = scan.getCentroids();
      if (!peaks.empty() && peaks[0].getRank() == 0)
      {
         throw std::invalid_argument("Ranks need to be assigned before scoring.");
      }

      auto matches = matchPeaks(ions, peaks);
      std::vector<int> matchesByPeakDepth(maxPeakDepth + 1, 0);

      for (const auto& match : matches)
      {
         int obsIndex = match.second;
         int rank = peaks[obsIndex].getRank();
         if (rank > maxPeakDepth)
         {
            continue;
         }
         // std::cout << "Match " << peaks[obsIndex].getMz() << ", " << peaks[obsIndex].getIntensity() << ", " << peaks[obsIndex].getRank() << "\n";

         matchesByPeakDepth[rank]++;
      }

      peptide.setIonsTotal(static_cast<int>(ions.size()));
      peptide.setIonsMatched(static_cast<int>(matches.size()));

      std::vector<PeakMatch>& peptideMatches = peptide.getMatches();
      peptideMatches.clear();

      for (const auto& match : matches)
      {
         PeakMatch peakMatch;
         peakMatch.theoMz = ions[match.first].getMz();
         peakMatch.obsMz = peaks[match.second].getMz();
         peakMatch.intensity = peaks[match.second].getIntensity();
         peakMatch.rank = peaks[match.second].getRank();
         peptideMatches.push_back(peakMatch);
      }

      peptide.setMatchesByDepth(matchesByPeakDepth);

      return calculateScore(peptide.getIonsTotal(), peptide.getIonsMatched(), matchesByPeakDepth, output);
   }

   double MOBScore::binomialCDFUpper(int trials, int successes, double p)
   {
      if (successes < 1)
      {
         return 1.0;
      }
      if (successes > trials)
      {
         throw std::invalid_argument("successes greater than trials");
      }
      if (successes < 0)
      {
         throw std::invalid_argument("negative successes");
      }
      if (trials < 0)
      {
         throw std::invalid_argument("negative trials");
      }
      if (trials == 0)
      {
         throw std::invalid_argument("no trials");
      }
      return Binomial::CDF(1.0 - p, trials, trials - successes);
   }

   std::unordered_map<int, int> MOBScore::matchPeaks(const std::vector<Centroid>& theo, const std::vector<Centroid>& obs)
   {
      double tol = options_.getTolerance();
      Mass::Units units = options_.getUnits();

      // One match per peak in the observed spectrum.
      // The match that will be kept is the one with the least
      // mass deviation.
      // Matches are saved as (theoIndex, obsIndex)
      std::vector<MOBPeakMatch> allMatches;
      std::unordered_map<int, int> obsToTheo;

      int obsIndex = 0;
      int prevObsStart = 0;

      for (int theoIndex = 0; theoIndex < static_cast<int>(theo.size()); ++theoIndex)
      {
         double theoMz = theo[theoIndex].getMz();
         double lowMz = 0.0;
         double highMz = 0.0;

         PeakMatcher::window(theoMz, tol, units, lowMz, highMz);

         // advance the obsIndex to the start of the window
         obsIndex = prevObsStart;
         while (obsIndex < static_cast<int>(obs.size()) && obs[obsIndex].getMz() < lowMz)
         {
            ++obsIndex;
         }
         prevObsStart = obsIndex;

         while (obsIndex < static_cast<int>(obs.size()) && obs[obsIndex].getMz() < highMz)
         {
            double error = std::abs(obs[obsIndex].getMz() - theoMz);
            MOBPeakMatch match;
            match.error = error;
            match.obsIndex = obsIndex;
            match.theoIndex = theoIndex;

            allMatches.push_back(match);
            obsToTheo[obsIndex] = theoIndex;
            ++obsIndex;
         }
      }

      // Sort to put matches with least error first.
      std::sort(allMatches.begin(), allMatches.end(),
                [](const MOBPeakMatch& a, const MOBPeakMatch& b)
      {
         return a.error < b.error;
      });

      // Grant matches to those with the least error first.
      std::unordered_map<int, int> matches;
      for (const auto& allMatch : allMatches)
      {
         obsIndex = allMatch.obsIndex;
         int theoIndex = allMatch.theoIndex;

         auto obsToTheoIt = obsToTheo.find(obsIndex);
         auto matchesIt = matches.find(theoIndex);

         if (obsToTheoIt != obsToTheo.end() && matchesIt == matches.end())
         {
            // store new theoretical-experimental pairs,
            // thus keeping only the best match per theoretical peak
            matches[theoIndex] = obsIndex;
            obsToTheo.erase(obsIndex);
         }
      }

      return matches;
   }

   double MOBScore::calculateScore(int ionsTotal, int ionsMatched, const std::vector<int>& matchesByPeakDepth, AScoreOutput& output)
   {
      double windowSize = options_.getWindow();
      int maxPeakDepth = options_.getMaxPeakDepth();

      double tolerance = options_.getTolerance();
      if (options_.getUnits() == Mass::Units::PPM)
      {
         // ppm*1e3/1e6 = Da at 1000 m/z
         tolerance /= 1000.0;
      }
      double pFactor = tolerance * 2.0 / windowSize;

      // Start of scoring
      double score1 = 0.0;
      double max_binomial = 0.0;
      double binomial_score = 0.0;
      double p = 0.0;
      int n_cum_matches = 0;
      int n_trials = ionsTotal;
      int peak_depth = 1;

      while (peak_depth <= maxPeakDepth && n_cum_matches < ionsMatched)
      {
         n_cum_matches += matchesByPeakDepth[peak_depth];
         p = std::min(0.999999, std::max(0.000001, static_cast<double>(peak_depth) * pFactor));
         binomial_score = -10.0 * std::log10(binomialCDFUpper(n_trials, n_cum_matches, p));

         if (binomial_score > max_binomial || binomial_score == 0.0)
         {
            max_binomial = binomial_score;
            peak_depth++;
         }
         else
         {
            score1 += max_binomial;
            n_trials -= (n_cum_matches - matchesByPeakDepth[peak_depth]);
            n_cum_matches = 0;
            max_binomial = 0.0;
         }
      }

      if (output.bestPeakDepth_ == 0)
      {
         output.bestPeakDepth_ = peak_depth - 1;
      }

      score1 += max_binomial;
      return score1;
   }

} // namespace AScoreProCpp
