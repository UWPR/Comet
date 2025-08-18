#include "Precursor.h"

namespace AScoreProCpp {

    Precursor::Precursor() :
        mz_(0.0),
        intensity_(0.0),
        charge_(0),
        originalMz_(0.0),
        originalCharge_(0),
        isolationMz_(0.0),
        isolationWidth_(0.0),
        isolationSpecificity_(0.0) {
    }

    Precursor::Precursor(double mz, double intensity, int charge) :
        mz_(mz),
        intensity_(intensity),
        charge_(charge),
        originalMz_(mz),
        originalCharge_(charge),
        isolationMz_(0.0),
        isolationWidth_(0.0),
        isolationSpecificity_(0.0) {
    }

    Precursor::Precursor(const Precursor& precursor) :
        mz_(precursor.mz_),
        intensity_(precursor.intensity_),
        charge_(precursor.charge_),
        originalMz_(precursor.originalMz_),
        originalCharge_(precursor.originalCharge_),
        isolationMz_(precursor.isolationMz_),
        isolationWidth_(precursor.isolationWidth_),
        isolationSpecificity_(precursor.isolationSpecificity_) {
    }

    double Precursor::getMz() const {
        return mz_;
    }

    void Precursor::setMz(double mz) {
        mz_ = mz;
    }

    double Precursor::getIntensity() const {
        return intensity_;
    }

    void Precursor::setIntensity(double intensity) {
        intensity_ = intensity;
    }

    int Precursor::getCharge() const {
        return charge_;
    }

    void Precursor::setCharge(int charge) {
        charge_ = charge;
    }

    double Precursor::getMh() const {
        return (mz_ * charge_) - (Mass::Proton * (charge_ - 1));
    }

    double Precursor::getOriginalMz() const {
        return originalMz_;
    }

    void Precursor::setOriginalMz(double originalMz) {
        originalMz_ = originalMz;
    }

    int Precursor::getOriginalCharge() const {
        return originalCharge_;
    }

    void Precursor::setOriginalCharge(int originalCharge) {
        originalCharge_ = originalCharge;
    }

    double Precursor::getIsolationMz() const {
        return isolationMz_;
    }

    void Precursor::setIsolationMz(double isolationMz) {
        isolationMz_ = isolationMz;
    }

    double Precursor::getIsolationWidth() const {
        return isolationWidth_;
    }

    void Precursor::setIsolationWidth(double isolationWidth) {
        isolationWidth_ = isolationWidth;
    }

    double Precursor::getIsolationSpecificity() const {
        return isolationSpecificity_;
    }

    void Precursor::setIsolationSpecificity(double isolationSpecificity) {
        isolationSpecificity_ = isolationSpecificity;
    }

} // namespace AScoreProCpp