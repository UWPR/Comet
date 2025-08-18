#include "AminoAcidMasses.h"

namespace AScoreProCpp {

    AminoAcidMasses::AminoAcidMasses() : nTermMass_(0), cTermMass_(0) {
        // Initialize the array with standard unmodified AA masses
        aaMasses_ = {
            71.03711381, // A
            0,           // B
            103.0091845, // C
            115.0269431, // D
            129.0425931, // E
            147.0684139, // F
            57.02146374, // G
            137.0589119, // H
            113.084064,  // I
            0,           // J
            128.0949631, // K
            113.084064,  // L
            131.0404847, // M
            114.0429275, // N
            0,           // O
            97.05276388, // P
            128.0585775, // Q
            156.1011111, // R
            87.03202844, // S
            101.0476785, // T
            0,           // U
            99.06841395, // V
            186.079313,  // W
            113.084064,  // X
            163.0633286, // Y
            0            // Z
        };
    }

    double AminoAcidMasses::getAminoAcidMass(char aa) const {
        // Convert character to array index (0-25 for A-Z)
        return aaMasses_[static_cast<int>(aa) - 65];
    }

    void AminoAcidMasses::modifyAminoAcidMass(char aa, double modMass) {
        aaMasses_[static_cast<int>(aa) - 65] += modMass;
    }

    double AminoAcidMasses::getNTermMass() const {
        return nTermMass_;
    }

    void AminoAcidMasses::modifyNTermMass(double modMass) {
        nTermMass_ += modMass;
    }

    double AminoAcidMasses::getCTermMass() const {
        return cTermMass_;
    }

    void AminoAcidMasses::modifyCTermMass(double modMass) {
        cTermMass_ += modMass;
    }

    AminoAcidMasses AminoAcidMasses::clone() const {
        // Create a new instance with same values
        AminoAcidMasses copy;
        copy.aaMasses_ = this->aaMasses_;
        copy.nTermMass_ = this->nTermMass_;
        copy.cTermMass_ = this->cTermMass_;
        return copy;
    }

} // namespace AScoreProCpp