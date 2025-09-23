#pragma once

#include <string>
#include <vector>
#include <limits>
#include "AScoreAPI.h"
#include "AScoreCentroid.h"
#include "AScoreOptions.h"
#include "AScoreOutput.h"
#include "AScorePeptide.h"
#include "AScoreScan.h"

namespace AScoreProCpp
{

   /**
    * A simplified interface for the AScore calculator to be exposed through a DLL
    */
   class ASCORE_API AScoreDllInterface
   {
private:
      // Default options for AScore calculation
      AScoreOptions CreateDefaultOptions() const;

      // Create a scan object from a vector of centroids and precursor m/z and charge
      Scan CreateScanFromCentroids(const std::vector<Centroid>& centroids, double precursorMz, int precursorCharge) const;

      // Create a scan object from a vector of centroids, scan m/z range, and precursor m/z and charge
      Scan CreateScanFromCentroids(const std::vector<Centroid>& centroids, double minMz, double maxMz, double precursorMz, int precursorCharge) const;

      // Parse a peptide string into a Peptide object
      Peptide ParsePeptideString(const std::string& peptideSequence, const AScoreOptions& options) const;

      // Calculate AScore with custom options
      AScoreOutput CalculateScore(
         const Scan& scan,
         Peptide& peptide,
         const AScoreOptions& options) const;

public:
      AScoreDllInterface();
      ~AScoreDllInterface();

      /**
       * Calculate AScore for a peptide sequence with the provided spectrum data
       *
       * @param peptideSequence The peptide sequence string with modifications (e.g. "K.M*LAES#DDS#GDEESVSQTDK.T")
       * @param peaks Vector of centroid peaks from the spectrum
       * @param precursorMz The precursor m/z value
       * @param precursorCharge The charge state of the precursor ion
       * @return AScoreOutput containing the results of the calculation
       */
      AScoreOutput CalculateScore(
         const std::string& peptideSequence,
         const std::vector<Centroid>& peaks,
         double precursorMz,
         int precursorCharge) const;

      /**
       * Calculate AScore with custom options
       *
       * @param peptideSequence The peptide sequence string with modifications
       * @param peaks Vector of centroid peaks from the spectrum
       * @param precursorMz The precursor m/z value
       * @param precursorCharge The charge state of the precursor ion
       * @param options Custom AScoreOptions to use for the calculation
       * @return AScoreOutput containing the results of the calculation
       */
      AScoreOutput CalculateScoreWithOptions(
         const std::string& peptideSequence,
         const std::vector<Centroid>& peaks,
         double precursorMz,
         int precursorCharge,
         const AScoreOptions& options) const;

      /**
       * Calculate AScore with custom options
       *
       * @param peptideSequence The peptide sequence string with modifications
       * @param peaks Vector of centroid peaks from the spectrum
       * @param minMz lower limit of the m/z range
       * @param maxMz upper limit of the m/z range
       * @param precursorMz The precursor m/z value
       * @param precursorCharge The charge state of the precursor ion
       * @param options Custom AScoreOptions to use for the calculation
       * @return AScoreOutput containing the results of the calculation
       */
      AScoreOutput CalculateScoreWithOptions(
         const std::string& peptideSequence,
         const std::vector<Centroid>& peaks,
         double minMz,
         double maxMz,
         double precursorMz,
         int precursorCharge,
         const AScoreOptions& options) const;

      /**
       * Get the default AScoreOptions
       *
       * @return A copy of the default AScoreOptions
       */
      AScoreOptions GetDefaultOptions() const;
   };

} // namespace AScoreProCpp
