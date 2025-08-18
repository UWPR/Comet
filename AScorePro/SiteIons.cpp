#include "SiteIons.h"
#include <cmath>

namespace AScoreProCpp {

    std::vector<Centroid> SiteIons::Filter(
        const std::vector<Centroid>& ions,
        const std::vector<Centroid>& remove) {

        std::vector<Centroid> output;

        // Use the same algorithm as the C# implementation
        int j = 0;
        const double delta = 1e-5;

        for (int i = 0; i < static_cast<int>(ions.size()); ++i) {
            // Skip elements in 'remove' that have m/z less than current ion - delta
            while (j < static_cast<int>(remove.size()) && remove[j].getMz() < ions[i].getMz() - delta) {
                ++j;
            }

            // If we found a matching ion in 'remove', skip this ion
            if (j < static_cast<int>(remove.size()) && std::abs(ions[i].getMz() - remove[j].getMz()) < delta) {
                continue;
            }

            // Add the ion to output (it's unique to the first list)
            Centroid centroid;
            centroid.setMz(ions[i].getMz());
            centroid.setIntensity(ions[i].getIntensity());
            output.push_back(centroid);
        }

        return output;
    }

} // namespace AScoreProCpp