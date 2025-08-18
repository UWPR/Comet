#pragma once

// Include all necessary dependencies
#include <string>
#include "Mass.h"
#include "API.h"

namespace AScoreProCpp {

    class ASCORE_API Precursor {
    public:
        // Constructors
        Precursor();
        Precursor(double mz, double intensity = 0, int charge = 0);
        Precursor(const Precursor& precursor);

        // Getters and setters
        double getMz() const;
        void setMz(double mz);

        double getIntensity() const;
        void setIntensity(double intensity);

        int getCharge() const;
        void setCharge(int charge);

        /**
         * Precursor M+H
         */
        double getMh() const;

        double getOriginalMz() const;
        void setOriginalMz(double originalMz);

        int getOriginalCharge() const;
        void setOriginalCharge(int originalCharge);

        double getIsolationMz() const;
        void setIsolationMz(double isolationMz);

        double getIsolationWidth() const;
        void setIsolationWidth(double isolationWidth);

        double getIsolationSpecificity() const;
        void setIsolationSpecificity(double isolationSpecificity);

    private:
        /**
         * The m/z of the precursor peak.
         * This is often the (putative) monoisotopic m/z of the molecule.
         */
        double mz_;

        /**
         * Intensity of the precursor peak.
         */
        double intensity_;

        /**
         * Charge state of the precursor.
         */
        int charge_;

        /**
         * The m/z of the precursor peak before reassignment.
         */
        double originalMz_;

        /**
         * The charge of the precursor before reassignment.
         */
        int originalCharge_;

        /**
         * The m/z that the instrument targeted for isolation.
         */
        double isolationMz_;

        /**
         * The size of the window that the instrument targeted for isolation.
         */
        double isolationWidth_;

        /**
         * Proportion of the intensity in the isolation window
         * that belongs to the precursor.
         *
         * This should be a value from zero to one.
         */
        double isolationSpecificity_;
    };

} // namespace AScoreProCpp