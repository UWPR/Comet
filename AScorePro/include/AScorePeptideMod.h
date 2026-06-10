#pragma once

#ifndef _ASCOREPEPTIDEMOD_H_
#define _ASCOREPEPTIDEMOD_H_

#include <string>
#include "AScoreAPI.h"

namespace AScoreProCpp
{

   class ASCORE_API PeptideMod
   {
public:
      PeptideMod();

      // Getters and setters
      char getSymbol() const;
      void setSymbol(char symbol);

      const std::string& getResidues() const;
      void setResidues(const std::string& residues);

      double getMass() const;
      void setMass(double mass);

      int getPosition() const;
      void setPosition(int position);

      bool getIsNTerm() const;
      void setIsNTerm(bool isNTerm);

      bool getIsCTerm() const;
      void setIsCTerm(bool isCTerm);

      /**
       * Returns true if the mod can be applied at the input amino acid.
       */
      bool applies(char aa) const;

      /**
       * Creates a copy of this object
       */
      PeptideMod clone() const;

private:
      // Symbol representing the modification in the annotated peptide.
      char symbol_;

      // Residues where the modification may be applied (e.g. "STY")
      std::string residues_;

      // Change in mass when the mod is applied.
      double mass_;

      // Stores the position along the peptide for the mod.
      int position_;

      // Set to true if the mod can be applied at the peptide n-terminus
      bool isNTerm_;

      // Set to true if the mod can be applied to the peptide c-terminus
      bool isCTerm_;
   };

} // namespace AScoreProCpp

#endif // _ASCOREPEPTIDEMOD_H_
