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

#ifndef _COMETALIGNMENT_H_
#define _COMETALIGNMENT_H_

#include "Common.h"
#include "CometDataInternal.h"
#include <vector>
#include <deque>
#include <utility>
#include <cstddef>

class CometMassSpecAligner
{
public:
   CometMassSpecAligner(int history_size = 20, double threshold = 2.5);
   double processRetentionMatch(double dQueryRetentionTime, double dCandidateReferenceTime);
   size_t getHistorySize() const;
   std::pair<double, double> getCurrentRegression() const;
   void reset();
   void setOutlierThreshold(double threshold);

private:
   int RetentionTimeMaxHistorySize;
   double RetentionTimeOutlierThreshold;
   int CurrentSpectrumIndex;

   std::pair<double, double> calculateLinearRegression(const std::vector<RetentionMatch>& matches) const;
   double calculateResidualStdDev(const std::vector<RetentionMatch>& matches, double slope, double intercept) const;
   bool isOutlier(const RetentionMatch& candidate, const std::vector<RetentionMatch>& ReferenceMatches) const;
};

#endif // _COMETALIGNMENT_H_
