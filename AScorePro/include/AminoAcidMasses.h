///////////////////////////////////////////////////////////////////////////////
//  Definitions for amino acid mass handling and modification utilities.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _AMINOACIDMASSES_H_
#define _AMINOACIDMASSES_H_

#include <array>
#include "API.h"

namespace AScoreProCpp {

    /**
     * Stores an instance of Amino Acid masses and defines
     * methods to apply static modifications to them.
     */
    class ASCORE_API AminoAcidMasses {
    public:
        AminoAcidMasses();

        /**
         * Returns an unmodified amino acid mass
         *
         * @param aa The uppercase character representing the amino acid
         * @return Mass
         */
        double getAminoAcidMass(char aa) const;

        /**
         * Change amino acid mass by adding a modification
         *
         * @param aa Uppercase character representing an amino acid
         * @param modMass The modification mass to apply
         */
        void modifyAminoAcidMass(char aa, double modMass);

        /**
         * Returns mass change to apply at peptide n-terminus
         */
        double getNTermMass() const;

        /**
         * Apply a mass modification to the peptide n-terminus
         */
        void modifyNTermMass(double modMass);

        /**
         * Returns mass change to apply at peptide c-terminus
         */
        double getCTermMass() const;

        /**
         * Apply a mass modification to the peptide c-terminus
         *
         * @param modMass The modification mass to apply
         */
        void modifyCTermMass(double modMass);

        /**
         * Copy the object
         */
        AminoAcidMasses clone() const;

    private:
        // Standard unmodified AA masses (26 letters of alphabet)
        std::array<double, 26> aaMasses_;

        // Mass change to apply at peptide n-terminus
        double nTermMass_;

        // Mass change to apply at peptide c-terminus
        double cTermMass_;
    };

} // namespace AScoreProCpp

#endif // _AMINOACIDMASSES_H_