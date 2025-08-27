#include "AScoreDllInterface.h"
#include "AScoreCalculator.h"
#include "PeptideBuilder.h"
#include "Precursor.h"

namespace AScoreProCpp
{
   AScoreDllInterface::AScoreDllInterface()
   {
      // Constructor implementation
   }

   AScoreDllInterface::~AScoreDllInterface()
   {
      // Destructor implementation
   }


   AScoreOptions AScoreDllInterface::CreateDefaultOptions() const
   {
      AScoreOptions options;

      // Ion series settings from JSON: ["b", "y", "nB", "nY"]
      // Map to bit flags: b=16, y=1024, nB=2, nY=4
      // Total: 16 + 1024 + 2 + 4 = 1046
      options.setIonSeries(1046);

      // Set ion series list
      std::vector<std::string> ionSeriesList = { "b", "y", "nB", "nY" };
      options.setIonSeriesList(ionSeriesList);

      // Peak depth settings
      options.setPeakDepth(0);
      options.setMaxPeakDepth(50);

      // Fragment matching tolerance
      options.setTolerance(0.3);
      options.setUnits(Mass::Units::DALTON);
      options.setUnitText("Da");

      // Window size for filtering peaks
      options.setWindow(70);

      // Enable low mass cutoff
      options.setLowMassCutoff(true);

      // Filter low intensity peaks
      options.setFilterLowIntensity(0);

      // Deisotoping type (empty string means no deisotoping)
      // options.setDeisotopingType("");

      // C-terminal settings
      options.setNoCterm(true);

      // Scoring options
      options.setUseMobScore(true);
      options.setUseDeltaAscore(true);

      // Target modification settings
      options.setSymbol('#'); // Phosphorylation symbol
      options.setResidues("STY"); // Phosphorylation residues

      // Max peptides and other limits
      options.setMaxPeptides(1000);
      // options.setMaxDiff(5); // From max_diff in JSON

      // Initialize other fields to default values from JSON
      options.setMz(0);
      options.setPeptide("");
      options.setScan(0);

      // Set up static modifications
      std::vector<PeptideMod> staticMods;

      // Carbamidomethyl cysteine (C +57.021464)
      PeptideMod carbamidoMod;
      carbamidoMod.setResidues("C");
      carbamidoMod.setMass(57.021464);
      carbamidoMod.setIsNTerm(false);
      carbamidoMod.setIsCTerm(false);
      staticMods.push_back(carbamidoMod);

      options.setStaticMods(staticMods);

      // Set up differential modifications
      std::vector<PeptideMod> diffMods;

      // Methionine oxidation (M* +15.994915)
      PeptideMod metOxMod;
      metOxMod.setSymbol('0');
      metOxMod.setResidues("M");
      metOxMod.setMass(15.994915);
      metOxMod.setIsNTerm(false);
      metOxMod.setIsCTerm(false);
      diffMods.push_back(metOxMod);

      // Phosphorylation (STY# +79.96633)
      PeptideMod phosphoMod;
      phosphoMod.setSymbol('1');
      phosphoMod.setResidues("STY");
      phosphoMod.setMass(79.96633);
      phosphoMod.setIsNTerm(false);
      phosphoMod.setIsCTerm(false);
      diffMods.push_back(phosphoMod);

      options.setDiffMods(diffMods);

      // Set up neutral loss
      NeutralLoss neutralLoss;
      neutralLoss.setMass(-97.9769);
      neutralLoss.setResidues("ST");
      options.setNeutralLoss(neutralLoss);

      // Apply static mods to AminoAcidMasses
      AminoAcidMasses& masses = options.getMasses();
      for (const auto& mod : staticMods)
      {
         const std::string& residues = mod.getResidues();
         if (!residues.empty())
         {
            for (char c : residues)
            {
               masses.modifyAminoAcidMass(c, mod.getMass());
            }
         }

         if (mod.getIsNTerm())
         {
            masses.modifyNTermMass(mod.getMass());
         }

         if (mod.getIsCTerm())
         {
            masses.modifyCTermMass(mod.getMass());
         }
      }

      return options;
   }


   Scan AScoreDllInterface::CreateScanFromCentroids(const std::vector<Centroid>& centroids, double precursorMz, int precursorCharge) const
   {
      // Set m/z range
      double minMz = std::numeric_limits<double>::max();
      double maxMz = 0.0;

      // Find the range and base peak
      for (const auto& centroid : centroids)
      {
         if (centroid.getMz() < minMz)
         {
            minMz = centroid.getMz();
         }
         if (centroid.getMz() > maxMz)
         {
            maxMz = centroid.getMz();
         }
      }

      minMz = minMz - 1; // Subtract 1 so we can find a match to the first peak
      maxMz = maxMz + 1; // Add 1 so we can find a match to the last peak

      return CreateScanFromCentroids(centroids, minMz, maxMz, precursorMz, precursorCharge);
   }

   Scan AScoreDllInterface::CreateScanFromCentroids(const std::vector<Centroid>& centroids, double minMz, double maxMz, double precursorMz, int precursorCharge) const
   {
      Scan scan;

      // Create a default scan object
      scan.setScanNumber(1);
      scan.setMsOrder(2); // MS2 scan

      // Set scan properties
      scan.setStartMz(minMz);
      scan.setEndMz(maxMz);
      scan.setLowestMz(minMz);
      scan.setHighestMz(maxMz);
      scan.setPeakCount(centroids.size());

      // Set the centroids
      scan.setCentroids(centroids);

      // Create precursor information
      Precursor precursor;
      precursor.setMz(precursorMz);

      // Set the provided charge state
      precursor.setCharge(precursorCharge);

      // Add precursor to scan
      std::vector<Precursor> precursors;
      precursors.push_back(precursor);
      scan.setPrecursors(precursors);

      return scan;
   }

   Peptide AScoreDllInterface::ParsePeptideString(const std::string& peptideSequence, const AScoreOptions& options) const
   {
      // Create a peptide builder using the diff mods from options
      PeptideBuilder builder(options.getDiffMods());

      // Parse the peptide string
      Peptide peptide = builder.build(peptideSequence);

      // Set precursor m/z if needed
      if (peptide.getPrecursorMz() <= 0.0 && !peptideSequence.empty())
      {
         // This is a placeholder - in a real implementation, you would calculate
         // the precursor m/z from the peptide sequence and modifications
         peptide.setPrecursorMz(500.0);
      }

      return peptide;
   }


   AScoreOutput AScoreDllInterface::CalculateScore(
      const std::string& peptideSequence,
      const std::vector<Centroid>& peaks,
      double precursorMz,
      int precursorCharge) const
   {

      // Create default options
      AScoreOptions options = CreateDefaultOptions();

      return CalculateScoreWithOptions(peptideSequence, peaks, precursorMz, precursorCharge, options);
   }


   AScoreOutput AScoreDllInterface::CalculateScoreWithOptions(
      const std::string& peptideSequence,
      const std::vector<Centroid>& peaks,
      double precursorMz,
      int precursorCharge,
      const AScoreOptions& options) const
   {
         // Create scan from centroids
      Scan scan = CreateScanFromCentroids(peaks, precursorMz, precursorCharge);

      // Parse peptide string
      Peptide peptide = ParsePeptideString(peptideSequence, options);

      // Set the precursor m/z from the parameter
      peptide.setPrecursorMz(precursorMz);

      return CalculateScore(scan, peptide, options);
   }

   AScoreOutput AScoreDllInterface::CalculateScoreWithOptions(
      const std::string& peptideSequence,
      const std::vector<Centroid>& peaks,
      double minMz,
      double maxMz,
      double precursorMz,
      int precursorCharge,
      const AScoreOptions& options) const
   {

      // Create scan from centroids
      Scan scan = CreateScanFromCentroids(peaks, minMz, maxMz, precursorMz, precursorCharge);

      // Parse peptide string
      Peptide peptide = ParsePeptideString(peptideSequence, options);

      // Set the precursor m/z from the parameter
      peptide.setPrecursorMz(precursorMz);

      return CalculateScore(scan, peptide, options);
   }

   AScoreOutput AScoreDllInterface::CalculateScore(
      const Scan& scan,
      Peptide& peptide,
      const AScoreOptions& options) const
   {
      // Create AScoreCalculator with the provided options
      AScoreCalculator calculator(options);

      // Run the calculation
      return calculator.Run(peptide, scan);
   }

   AScoreOptions AScoreDllInterface::GetDefaultOptions() const
   {
      return CreateDefaultOptions();
   }

} // namespace AScoreProCpp