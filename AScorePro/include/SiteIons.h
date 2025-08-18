#pragma once

#include "Centroid.h"
#include <vector>

namespace AScoreProCpp {

    /**
     * Utility class for filtering ion masses to determine site-specific ions
     */
    class SiteIons {
    public:
        /**
         * Filters two ion lists to find ions that are unique to the first list
         *
         * @param ions First ion list
         * @param remove Second ion list with ions to remove from the first list
         * @return Vector of ions that are in ions but not in remove
         */
        static std::vector<Centroid> Filter(
            const std::vector<Centroid>& ions,
            const std::vector<Centroid>& remove);
    };

} // namespace AScoreProCpp
