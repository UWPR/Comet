#include "TopIonsFilter.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace AScoreProCpp
{

   TopIonsFilter::TopIonsFilter(int depth, double window)
      : depth_(depth), window_(window)
   {
   }

   void TopIonsFilter::Filter(Scan& scan)
   {
      std::vector<Centroid>& peaks = scan.getCentroids();

      if (peaks.empty())
      {
         return;
      }

      // Sort peaks by m/z (ascending) to ensure proper window processing
      std::sort(peaks.begin(), peaks.end(),
                [](const Centroid& a, const Centroid& b)
      {
         return a.getMz() < b.getMz();
      });

      // Rank peaks in each window based on intensity
      int start = 0;
      int i = 0;
      double currentWindowMin = std::floor(peaks[start].getMz() / window_) * window_;
      double currentWindowMax = currentWindowMin + window_;

      while (true)
      {
         if (i == peaks.size() || peaks[i].getMz() > currentWindowMax)
         {
            // Found the m/z range of a window
            // Sort window by intensity (descending)
            std::sort(peaks.begin() + start, peaks.begin() + i,
                      [](const Centroid& a, const Centroid& b)
            {
               return a.getIntensity() > b.getIntensity();
            });

            // Assign ranks to peaks in this window
            int rank = 1;
            for (int j = start; j != i; ++j)
            {
               Centroid peak = peaks[j];
               peak.setRank(rank++);
               peaks[j] = peak;
            }

            if (i == peaks.size())
            {
               break;
            }

            // Move to next window
            start = i;
            currentWindowMin = std::floor(peaks[start].getMz() / window_) * window_;
            currentWindowMax = currentWindowMin + window_;
         }
         ++i;
      }

      // Sort back by m/z (ascending)
      std::sort(peaks.begin(), peaks.end(),
                [](const Centroid& a, const Centroid& b)
      {
         return a.getMz() < b.getMz();
      });

      // Keep only top ranked peaks
      std::vector<Centroid> filteredPeaks;
      for (const auto& peak : peaks)
      {
         if (peak.getRank() <= depth_)
         {
            filteredPeaks.push_back(peak);
         }
      }

      // Replace the peaks in the scan
      peaks = filteredPeaks;
      /*for (const auto& peak : peaks) {
          int rank = peak.getRank();
          std::cout << "Peak " << peak.getMz() << ", " << peak.getIntensity() << ", " << peak.getRank() << "\n";
      }*/
   }

} // namespace AScoreProCpp