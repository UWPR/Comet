// Copyright 2025 Jimmy Eng
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

#include "CometAlignment.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>
#include <cstdio>  // For printf in example

RetentionMatch::RetentionMatch(double dQueryTime, double dReferenceTime, int iSpectrumIndex)
   : dQueryTime(dQueryTime), dReferenceTime(dReferenceTime), iSpectrumIndex(iSpectrumIndex)
{
}

CometMassSpecAligner::CometMassSpecAligner(int history_size, double threshold)
   : RetentionTimeMaxHistorySize(history_size), RetentionTimeOutlierThreshold(threshold), CurrentSpectrumIndex(0)
{
   if (history_size < 3)
   {
      throw std::invalid_argument("History size must be at least 3");
   }
}

std::pair<double, double> CometMassSpecAligner::calculateLinearRegression(const std::vector<RetentionMatch>& matches) const
{
   if (matches.size() < 3)
   {
      throw std::runtime_error("Insufficient data for regression");
   }

   // First pass: calculate initial regression to identify outliers
   double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
   int n = static_cast<int>(matches.size());

   for (const auto& match : matches)
   {
      double x = match.dQueryTime;
      double y = match.dReferenceTime;
      sum_x += x;
      sum_y += y;
      sum_xy += x * y;
      sum_x2 += x * x;
   }

   double initial_slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
   double initial_intercept = (sum_y - initial_slope * sum_x) / n;

   // Calculate residual standard deviation for outlier detection
   double residual_std = calculateResidualStdDev(matches, initial_slope, initial_intercept);

   if (residual_std == 0.0)
   {
      // If no variation in residuals, return the initial regression
      return std::make_pair(initial_slope, initial_intercept);
   }

   // Second pass: filter out outliers and recalculate regression
   std::vector<RetentionMatch> filtered_matches;
   filtered_matches.reserve(matches.size());

   for (const auto& match : matches)
   {
      double predicted = initial_slope * match.dQueryTime + initial_intercept;
      double residual = std::abs(match.dReferenceTime - predicted);

      // Use the same threshold logic as in isOutlier method
      if (residual <= (RetentionTimeOutlierThreshold * residual_std))
      {
         filtered_matches.push_back(match);
      }
   }

   // If too few points remain after filtering, use the original set
   if (filtered_matches.size() < 3)
   {
      return std::make_pair(initial_slope, initial_intercept);
   }

   // Recalculate regression with filtered data
   sum_x = sum_y = sum_xy = sum_x2 = 0;
   n = static_cast<int>(filtered_matches.size());

   for (const auto& match : filtered_matches)
   {
      double x = match.dQueryTime;
      double y = match.dReferenceTime;
      sum_x += x;
      sum_y += y;
      sum_xy += x * y;
      sum_x2 += x * x;
   }

   double filtered_slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
   double filtered_intercept = (sum_y - filtered_slope * sum_x) / n;

   return std::make_pair(filtered_slope, filtered_intercept);
}

double CometMassSpecAligner::calculateResidualStdDev(const std::vector<RetentionMatch>& matches, double slope, double intercept) const
{
   if (matches.size() < 3)
   {
      return 0.0;
   }

   std::vector<double> residuals;
   residuals.reserve(matches.size());

   for (const auto& match : matches)
   {
      double predicted = slope * match.dQueryTime + intercept;
      residuals.push_back(std::abs(match.dReferenceTime - predicted));
   }

   double mean_residual = std::accumulate(residuals.begin(), residuals.end(), 0.0) / residuals.size();

   double variance = 0.0;
   for (double residual : residuals)
   {
      variance += (residual - mean_residual) * (residual - mean_residual);
   }
   variance /= (residuals.size() - 1);

   return std::sqrt(variance);
}

bool CometMassSpecAligner::isOutlier(const RetentionMatch& candidate, const std::vector<RetentionMatch>& ReferenceMatches) const
{
   if (ReferenceMatches.size() < 3)
   {
      return false;
   }

   std::pair<double, double> regression_result = calculateLinearRegression(ReferenceMatches);
   double slope = regression_result.first;
   double intercept = regression_result.second;
   double residual_std = calculateResidualStdDev(ReferenceMatches, slope, intercept);

   if (residual_std == 0.0)
   {
      return false;
   }

   double predicted = slope * candidate.dQueryTime + intercept;
   double residual = std::abs(candidate.dReferenceTime - predicted);

   return residual > (RetentionTimeOutlierThreshold * residual_std);
}

double CometMassSpecAligner::processRetentionMatch(double dQueryRetentionTime, double dCandidateReferenceTime)
{
   bool bDebug = false;  // Set to true for debugging output

   if ( fabs(dQueryRetentionTime - 1906.24) < 0.4)
   {
      bDebug = true;
   }

   CurrentSpectrumIndex++;

   RetentionMatch candidate(dQueryRetentionTime, dCandidateReferenceTime, CurrentSpectrumIndex);

   std::vector<RetentionMatch> RetentionRecentMatches(RetentionMatchHistory.begin(), RetentionMatchHistory.end());

   RetentionMatchHistory.push_back(candidate);
   if (RetentionMatchHistory.size() > static_cast<size_t>(RetentionTimeMaxHistorySize))
   {
      RetentionMatchHistory.pop_front();
   }

   if (RetentionRecentMatches.size() < 10)
   {
      return dCandidateReferenceTime;
   }


   std::pair<double, double> regression_result = calculateLinearRegression(RetentionRecentMatches);
   double predicted_time = regression_result.first * dQueryRetentionTime + regression_result.second;  // .first is slope, second is intercept
   return predicted_time;
}

size_t CometMassSpecAligner::getHistorySize() const
{
   return RetentionMatchHistory.size();
}

std::pair<double, double> CometMassSpecAligner::getCurrentRegression() const
{
   if (RetentionMatchHistory.size() < 2)
   {
      return std::make_pair(0.0, 0.0);
   }

   std::vector<RetentionMatch> RetentionRecentMatches(RetentionMatchHistory.begin(), RetentionMatchHistory.end());
   std::pair<double, double> regression_result = calculateLinearRegression(RetentionRecentMatches);

   return regression_result;
}

void CometMassSpecAligner::reset()
{
   RetentionMatchHistory.clear();
   CurrentSpectrumIndex = 0;
}

void CometMassSpecAligner::setOutlierThreshold(double threshold)
{
   RetentionTimeOutlierThreshold = threshold;
}

/*
// Example usage function
void example_usage()
{
   CometMassSpecAligner aligner(15, 2.0);

   std::vector<std::pair<double, double>> test_data =
   {
       {1.0, 1.1},
       {2.0, 2.2},
       {3.0, 3.1},
       {4.0, 4.2},
       {5.0, 5.1},
       {6.0, 8.5},
       {7.0, 7.3},
       {8.0, 8.4}
   };

   for (const auto& data_point : test_data)
   {
      double dQueryTime = data_point.first;
      double ref_time = data_point.second;
      double result = aligner.processRetentionMatch(dQueryTime, ref_time);

      std::pair<double, double> regression_params = aligner.getCurrentRegression();
      double slope = regression_params.first;
      double intercept = regression_params.second;

      printf("Query: %.2f, Candidate: %.2f, Result: %.2f, Regression: y=%.3fx+%.3f\n",
         dQueryTime, ref_time, result, slope, intercept);
   }
}
*/
