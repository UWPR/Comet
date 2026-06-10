#include "AScoreIntensityFilter.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace AScoreProCpp
{

   IntensityFilter::IntensityFilter(double fractionToRemove)
      : fractionToRemove_(fractionToRemove)
   {
   }

   void IntensityFilter::Filter(Scan& scan)
   {
      std::vector<Centroid>& centroids = scan.getCentroids();

      // Check if we need to filter
      if (fractionToRemove_ < 0.01 || centroids.size() < 4)
      {
         return;
      }

      // Sort peaks by intensity (ascending)
      std::vector<Centroid> sortedPeaks = centroids;
      std::sort(sortedPeaks.begin(), sortedPeaks.end(),
                [](const Centroid& a, const Centroid& b)
      {
         return a.getIntensity() < b.getIntensity();
      });

      // Find the intensity threshold for filtering
      int cutoffIndex = static_cast<int>(std::floor(sortedPeaks.size() * fractionToRemove_));
      if (cutoffIndex >= sortedPeaks.size())
      {
         return;
      }

      double minIntensity = sortedPeaks[cutoffIndex].getIntensity();

      // Keep only peaks above the intensity threshold
      std::vector<Centroid> filteredPeaks;
      for (const auto& peak : centroids)
      {
         if (peak.getIntensity() > minIntensity)
         {
            filteredPeaks.push_back(peak);
         }
      }

      // Replace the peaks in the scan
      centroids = filteredPeaks;
   }

} // namespace AScoreProCpp
