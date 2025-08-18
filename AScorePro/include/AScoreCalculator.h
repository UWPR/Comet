#pragma once

#include "AScoreOptions.h"
#include "AScoreOutput.h"
#include "Peptide.h"
#include "Scan.h"
#include <vector>

namespace AScoreProCpp {

    /**
     * The AScoreCalculator class stores the configuration for the
     * instance of AScore and can run AScore for a peptide and scan.
     */
    class AScoreCalculator {
    private:
        AScoreOptions options_;

        /**
         * Pre-processes a scan using the configured filters
         */
        void PreProcessScan(Scan& scan);

    public:
        /**
         * Constructor with AScoreOptions
         */
        explicit AScoreCalculator(const AScoreOptions& options);

        /**
         * Get a copy of the options
         */
        AScoreOptions GetOptions() const;

        /**
         * Runs AScore for a single peptide
         */
        AScoreOutput Run(Peptide& peptide, const Scan& inputScan);
    };

} // namespace AScoreProCpp
