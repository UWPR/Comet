#pragma once

#ifndef _PEPTIDEBUILDER_H_
#define _PEPTIDEBUILDER_H_

#include <string>
#include <unordered_map>
#include <vector>
#include "API.h"
#include "Peptide.h"
#include "PeptideMod.h"

namespace AScoreProCpp
{

   /**
    * This parser is used to read in peptides from the input csv.
    * Input strings should be the annotated peptide sequence.
    * Differential mods need to be passed into the constructor to
    * define the mod symbols.
    */
   class ASCORE_API PeptideBuilder
   {
public:
      /**
       * Constructor. Pass in the differential mods to
       * define which mods may appear in the peptide sequence.
       *
       * @param mods Definitions of the differential mods
       */
      PeptideBuilder(const std::vector<PeptideMod>& mods);

      /**
       * Parses the peptides, separating the annotated sequence
       * to the amino acids and the modifications. The input string
       * may have flanking residues and modification information
       * Ex. "K.M*LAES#DDS#GDEESVSQTDK.T"
       *
       * @param peptideStr The annotated peptide string.
       * @return The unserialized peptide
       */
      Peptide build(const std::string& peptideStr) const;

private:
      std::unordered_map<char, PeptideMod> mods_;
   };

} // namespace AScoreProCpp

#endif // _PEPTIDEBUILDER_H_
