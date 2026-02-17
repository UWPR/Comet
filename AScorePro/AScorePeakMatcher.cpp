#include <cmath>
#include "AScorePeakMatcher.h"
#include "AScoreScan.h"

namespace AScoreProCpp
{

   int PeakMatcher::match(const Scan& scan, double targetMz, double tolerance, int tolUnits)
   {
      int i = nearestIndex(scan.getCentroids(), targetMz);

      int count = static_cast<int>(scan.getCentroids().size());
      bool foundNext = false;
      double errorNext = 0;

      if (i < count)
      {
         foundNext = true;
         errorNext = std::abs(scan.getCentroids()[i].getMz() - targetMz);
      }

      bool foundPrevious = false;
      double errorPrevious = 0;

      if (i > 0)
      {
         foundPrevious = true;
         errorPrevious = std::abs(scan.getCentroids()[i - 1].getMz() - targetMz);
      }

      if (!foundNext && !foundPrevious)
      {
         return -1;
      }

      if (!foundNext || (foundPrevious && errorPrevious < errorNext))
      {
         if (withinError(targetMz, scan.getCentroids()[i - 1].getMz(), tolerance, tolUnits))
         {
            return i - 1;
         }
      }
      else if (!foundPrevious || (foundNext && errorNext < errorPrevious))
      {
         if (withinError(targetMz, scan.getCentroids()[i].getMz(), tolerance, tolUnits))
         {
            return i;
         }
      }

      return -1;
   }

   int PeakMatcher::mostIntenseIndex(const Scan& scan, double targetMz, double tolerance, int tolUnits)
   {
      double lowMz = targetMz - tolerance;
      double highMz = targetMz + tolerance;

      if (tolUnits == PPM)
      {
         double delta = tolerance * targetMz / 1000000;
         lowMz = targetMz - delta;
         highMz = targetMz + delta;
      }

      const auto& peaks = scan.getCentroids();
      int i = PeakMatcher::nearestIndex(peaks, lowMz);

      if (i < static_cast<int>(peaks.size()) && peaks[i].getMz() < lowMz)
      {
         ++i;
      }

      double maxIntensity = 0;
      int bestIndex = -1;

      for (; i < static_cast<int>(peaks.size()) && peaks[i].getMz() < highMz; ++i)
      {
         if (peaks[i].getIntensity() > maxIntensity)
         {
            maxIntensity = peaks[i].getIntensity();
            bestIndex = i;
         }
      }

      return bestIndex;
   }

   int PeakMatcher::nearestIndex(const std::vector<Centroid>& peaks, double target)
   {
      int low = 0;
      int high = static_cast<int>(peaks.size()) - 1;
      int mid = 0;

      while (low < high)
      {
         mid = (high + low) / 2;

         if (peaks[mid].getMz() < target)
         {
            low = mid + 1;
         }
         else
         {
            high = mid;
         }
      }

      return low;
   }

   bool PeakMatcher::withinError(double theoretical, double observed, double tolerance, int tolUnits)
   {
      switch (tolUnits)
      {
      case PeakMatcher::DALTON:
         return std::abs(theoretical - observed) < tolerance;
      case PeakMatcher::PPM:
         return std::abs(getPpm(theoretical, observed)) < tolerance;
      default:
         break;
      }
      return false;
   }

   bool PeakMatcher::withinError(double theoretical, double observed, double tolerance, Mass::Units tolUnits, double& error)
   {
      if (tolUnits == Mass::Units::PPM)
      {
         error = std::abs(getPpm(theoretical, observed));
      }
      else
      {
         error = std::abs(theoretical - observed);
      }

      return error < tolerance;
   }

   void PeakMatcher::window(double mass, double tolerance, Mass::Units units, double& lowMz, double& highMz)
   {
      if (units == Mass::Units::PPM)
      {
         double e = (mass * tolerance / 1000000.0);
         lowMz = mass - e;
         highMz = mass + e;
         return;
      }

      lowMz = mass - tolerance;
      highMz = mass + tolerance;
   }

   double PeakMatcher::getPpm(double theoretical, double observed)
   {
      return 1000000 * (theoretical - observed) / theoretical;
   }

} // namespace AScoreProCpp
