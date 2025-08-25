#pragma once

#ifndef _NEUTRALLOSSFITLER_H_
#define _NEUTRALLOSSFILTER_H_

#include "PeakMatcher.h"
#include "Mass.h"
#include "Scan.h"

namespace AScoreProCpp
{

/**
 * Removes peaks from the scan that match the precursor peak
 * minus a neutral loss of phospho.
 */
class NeutralLossFilter
{
private:
   double m_Tolerance;
   Mass::Units m_Units;

   // Constants for neutral loss masses
   static constexpr double NL_MASS1 = 79.96633;
   static constexpr double NL_MASS2 = 97.97689;

public:
   /**
    * Initialize the filter.
    *
    * Pass in the peak match tolerance when searching for
    * precursor neutral losses.
    */
   NeutralLossFilter(double tolerance, Mass::Units units)
      : m_Tolerance(tolerance), m_Units(units) {}

   /**
    * Removes precursor neutral loss peaks from the scan.
    * The peak list in the scan is modified.
    */
   void Filter(Scan& scan)
   {
      if (scan.getPrecursors().empty())
      {
         return;
      }

      const auto& precursor = scan.getPrecursors()[0];
      if (precursor.getMz() < 1 || precursor.getCharge() == 0)
      {
         return;
      }

      // Remove neutral loss 1
      double targetMz = precursor.getMz() - (NL_MASS1 / precursor.getCharge());
      int i;
      while ((i = PeakMatcher::match(scan, targetMz, m_Tolerance, static_cast<int>(m_Units))) != -1)
      {
         std::vector<Centroid>& centroids = scan.getCentroids();
         centroids.erase(centroids.begin() + i);
      }

      // Remove neutral loss 2
      targetMz = precursor.getMz() - (NL_MASS2 / precursor.getCharge());
      while ((i = PeakMatcher::match(scan, targetMz, m_Tolerance, static_cast<int>(m_Units))) != -1)
      {
         std::vector<Centroid>& centroids = scan.getCentroids();
         centroids.erase(centroids.begin() + i);
      }
   }
};

} // namespace AScoreProCpp

#endif // _NEUTRALLOSSFILTER_H_
