#pragma once

#include <vector>
#include "Mass.h"
#include "Centroid.h"

namespace AScoreProCpp {

    // Forward declaration
    class Scan;

    class PeakMatcher {
    public:
        static const int PPM = 1;
        static const int DALTON = 2;

        /**
         * Find a matching peak in the scan for the target m/z.
         *
         * @param scan The scan containing the peaks to search
         * @param targetMz The theoretical m/z to match
         * @param tolerance The tolerance for matching
         * @param tolUnits Units can be DALTON or PPM
         * @return The index of the matched peak, or -1 if no match found
         */
        static int match(const Scan& scan, double targetMz, double tolerance, int tolUnits);

        /**
         * Returns the index of the most intense peak in the window.
         * Returns -1 if none is found.
         *
         * @param scan The scan that will be searched
         * @param targetMz The center of the window where peaks will be considered
         * @param tolerance window will be +/- this value
         * @param tolUnits Units can be DALTON or PPM
         * @return The index of the most intense peak in the window, or -1 if none found
         */
        static int mostIntenseIndex(const Scan& scan, double targetMz, double tolerance, int tolUnits);

        /**
         * Binary search to find the nearest peak index to the target m/z
         *
         * @param peaks Vector of centroids ordered by m/z
         * @param target Target m/z
         * @return Index of the nearest peak
         */
        static int nearestIndex(const std::vector<Centroid>& peaks, double target);

        /**
         * Check if observed m/z is within tolerance of theoretical m/z
         *
         * @param theoretical Theoretical m/z
         * @param observed Observed m/z
         * @param tolerance Tolerance value
         * @param tolUnits Units of tolerance (PPM or DALTON)
         * @return True if within tolerance
         */
        static bool withinError(double theoretical, double observed, double tolerance, int tolUnits);

        /**
         * Check if observed m/z is within tolerance of theoretical m/z and return the error
         *
         * @param theoretical Theoretical m/z
         * @param observed Observed m/z
         * @param tolerance Tolerance value
         * @param tolUnits Units of tolerance
         * @param error Output parameter for the error value
         * @return True if within tolerance
         */
        static bool withinError(double theoretical, double observed, double tolerance, Mass::Units tolUnits, double& error);

        /**
         * Calculate window bounds for a mass and tolerance
         *
         * @param mass The mass at the center of the window
         * @param tolerance The tolerance value
         * @param units The units of the tolerance (PPM or DALTON)
         * @param lowMz Output parameter for lower bound of the window
         * @param highMz Output parameter for upper bound of the window
         */
        static void window(double mass, double tolerance, Mass::Units units, double& lowMz, double& highMz);

        /**
         * Calculate PPM error between theoretical and observed m/z
         *
         * @param theoretical Theoretical m/z
         * @param observed Observed m/z
         * @return PPM error
         */
        static double getPpm(double theoretical, double observed);
    };

} // namespace AScoreProCpp
