#pragma once
#include "API.h"

namespace AScoreProCpp {

    /**
     * For pulling spectral information from API scans
     */
    class ASCORE_API Centroid {
    public:
        Centroid() :
            mz_(0.0),
            intensity_(0.0),
            baseline_(0.0),
            noise_(0.0),
            charge_(0),
            rank_(0) {}

        Centroid(double mz, double intensity, double baseline = 0, double noise = 0) :
            mz_(mz),
            intensity_(intensity),
            baseline_(baseline),
            noise_(noise),
            charge_(0),
            rank_(0) {}

        // Getters and setters
        double getMz() const { return mz_; }
        void setMz(double mz) { mz_ = mz; }

        double getBaseline() const { return baseline_; }
        void setBaseline(double baseline) { baseline_ = baseline; }

        double getIntensity() const { return intensity_; }
        void setIntensity(double intensity) { intensity_ = intensity; }

        double getNoise() const { return noise_; }
        void setNoise(double noise) { noise_ = noise; }

        int getCharge() const { return charge_; }
        void setCharge(int charge) { charge_ = charge; }

        int getRank() const { return rank_; }
        void setRank(int rank) { rank_ = rank; }

    private:
        // Centroid m/z
        double mz_;

        // Baseline
        double baseline_;

        // Centroid intensity
        double intensity_;

        // Noise level read from the instrument.
        double noise_;

        // Detected charge of the ion
        int charge_;

        // Intensity rank of the peak. Can be assigned
        // within a specific window inside the scan.
        int rank_;
    };

} // namespace AScoreProCpp