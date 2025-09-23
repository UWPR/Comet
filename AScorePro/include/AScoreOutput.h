#pragma once

#ifndef _ASCOREOUTPUT_H_
#define _ASCOREOUTPUT_H_

#include <vector>
#include <memory>
#include "AScoreAPI.h"
#include "AScoreOptions.h"
#include "AScorePeptide.h"
#include "AScoreScan.h"
#include "AScoreSiteScore.h"


namespace AScoreProCpp
{

   /**
    * Class to store the output results from AScore algorithm
    */
   class ASCORE_API AScoreOutput
   {
   public:
      AScoreOutput() : modCount_(0), bestPeakDepth_(0), bestPeptideScore_(0.0) {}

      /**
       * The input options to AScore
       */
      AScoreOptions options;

      /**
       * The number of mods on the original peptide.
       */
      int modCount_;

      /**
       * The peak depth that was used for site scoring
       */
      int bestPeakDepth_;

      /**
       * From the peptide that produces the best score.
       * Used with bestPeakDepth_
       */
      double bestPeptideScore_;

      /**
       * The spectrum that the peptide was scored against.
       */
      Scan scan;

      /**
       * Scored peptides with different arrangement of modifications.
       */
      std::vector<Peptide> peptides;

      /**
       * Site scores for the top peptide.
       */
      std::vector<SiteScore> sites;
   };

} // namespace AScoreProCpp

#endif // _ASCOREOUTPUT_H_
