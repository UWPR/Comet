#pragma once

#include "PeakMatcher.h"
#include "Mass.h"
#include "Scan.h"

namespace AScoreProCpp {

    /**
     * The water loss filter removes precursor peak
     * 1x and 2x water losses from the spectrum.
     */
    class WaterLossFilter {
    private:
        double m_Tolerance;
        Mass::Units m_Units;

    public:
        /**
         * Initialize the filter.
         *
         * Pass in the peak match tolerance when searching for water losses.
         */
        WaterLossFilter(double tolerance, Mass::Units units)
            : m_Tolerance(tolerance), m_Units(units) {}

        /**
         * Removes precursor water loss peaks from the scan.
         * Using the precursor m/z value in the scan, this method
         * searches for peaks subtracting 1x and 2x water loss and removes them.
         * The peak list in the scan is modified.
         */
        void Filter(Scan& scan) {
            if (scan.getPrecursors().empty()) {
                return;
            }

            const auto& precursor = scan.getPrecursors()[0];
            if (precursor.getMz() < 1 || precursor.getCharge() == 0) {
                return;
            }

            // Remove 1x water loss
            double targetMz = precursor.getMz() - (Mass::Water / precursor.getCharge());
            int i;
            while ((i = PeakMatcher::match(scan, targetMz, m_Tolerance, static_cast<int>(m_Units))) != -1) {
                std::vector<Centroid>& centroids = scan.getCentroids();
                centroids.erase(centroids.begin() + i);
            }

            // Remove 2x water loss
            targetMz = precursor.getMz() - (2 * Mass::Water / precursor.getCharge());
            while ((i = PeakMatcher::match(scan, targetMz, m_Tolerance, static_cast<int>(m_Units))) != -1) {
                std::vector<Centroid>& centroids = scan.getCentroids();
                centroids.erase(centroids.begin() + i);
            }
        }
    };

} // namespace AScoreProCpp
