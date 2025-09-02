#pragma once

#ifndef _ASCOREPEPTIDEGENERATOR_H_
#define _ASCOREPEPTIDEGENERATOR_H_

#include <vector>
#include <list>
#include "AScoreAminoAcidMasses.h"
#include "AScoreCentroid.h"
#include "AScoreCombinationIterator.h"
#include "AScoreMass.h"
#include "AScorePeptide.h"
#include "AScorePeptideMod.h"

namespace AScoreProCpp
{

   /**
    * Generates peptide variants with modifications at different positions.
    */
   class PeptideGenerator
   {
public:
      /**
       * Constructor initializes the peptide generator
       *
       * @param peptide Base peptide to generate modifications for
       * @param targetMod The modification to apply at different positions
       * @param aaMasses Amino acid mass reference
       */
      PeptideGenerator(const Peptide& peptide, const PeptideMod& targetMod, const AminoAcidMasses& aaMasses);

      /**
       * Checks if all modification combinations have been enumerated
       *
       * @return True if no more combinations are available
       */
      bool atEnd() const;

      /**
       * Advances to the next modification combination
       */
      void next();

      /**
       * Sets a neutral loss modification to apply
       *
       * @param mass Mass of the neutral loss
       * @param residues Residues that can have neutral loss
       */
      void setNeutralLossMod(double mass, const std::string& residues);

      /**
       * Sets the current index in the combinations list
       *
       * @param i Index to set
       */
      void setIndex(int i);

      /**
       * Returns the fragment ions for the peptide with the current
       * arrangement of mods.
       *
       * @param ionSeriesFlags The selected ion series as flags (bitwise OR of Mass::IonSeries values)
       * @param maxCharge Max fragment charge to consider (inclusive)
       * @param minMz min m/z for fragment ions
       * @param maxMz max m/z for fragment ions
       * @return list of peaks sorted by m/z in ascending order
       */
      std::vector<Centroid> getMassList(int ionSeriesFlags, int maxCharge, double minMz, double maxMz);

      /**
       * Returns the peptide with the current arrangement of mods.
       *
       * @return Current peptide with modifications
       */
      Peptide getPeptide();

private:
      /**
       * Initialize the internal data
       */
      void init();

      /**
       * Calculate ion m/z from mass and charge
       *
       * @param mass The mass of the ion
       * @param charge The charge state
       * @return m/z value
       */
      double ionMz(double mass, int charge) const;

      /**
       * Insert an ion m/z into a list if it falls within a range
       *
       * @param ions The list to insert into
       * @param minMz Minimum m/z to insert
       * @param maxMz Maximum m/z to insert
       * @param mz The m/z value to insert
       */
      void insertInRange(std::vector<Centroid>& ions, double minMz, double maxMz, double mz);

      double neutralLossMass_;
      std::string neutralLossResidues_;
      bool useNeutralLoss_;

      /**
       * Stores the mass at each AA position
       */
      std::vector<double> aaMasses_;

      /**
       * Stores the masses at each AA position plus any
       * modification masses.
       */
      std::vector<double> modMasses_;

      /**
       * Each list is a distinct combination of mods.
       * Values specify the position to apply the mods.
       */
      std::vector<std::vector<int>> combinations_;

      /**
       * Peptide to generate mod combinations.
       */
      Peptide basePeptide_;

      /**
       * The index of the generated mod combinations.
       */
      int index_;

      /**
       * Maximum number of mod combinations to generate.
       */
      int maxPeptides_;

      AminoAcidMasses masses_;
      PeptideMod targetMod_;
   };

} // namespace AScoreProCpp

#endif // _ASCOREPEPTIDEGENERATOR_H_
