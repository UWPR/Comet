#include <cmath>
#include <algorithm>
#include <vector>
#include <memory>
#include "AScoreCalculator.h"
#include "AScoreSiteIons.h"
#include "AScoreIntensityFilter.h"
#include "AScoreTopIonsFilter.h"
#include "AScoreWaterLossFilter.h"
#include "AScoreNeutralLossFilter.h"
#include "AScoreMOBScore.h"
#include "AScoreUnifiedPeptideGenerator.h"


namespace AScoreProCpp
{

   AScoreCalculator::AScoreCalculator(const AScoreOptions& options)
      : options_(options)
   {
   }

   AScoreOptions AScoreCalculator::GetOptions() const
   {
      return options_;
   }

   AScoreOutput AScoreCalculator::Run(Peptide& peptide, const Scan& inputScan)
   {
      AScoreOutput output;
      output.options = options_; // Using copy constructor

      // Clone the input scan and set precursor m/z
      Scan scan = inputScan;
      if (!scan.getPrecursors().empty())
      {
         scan.getPrecursors()[0].setMz(peptide.getPrecursorMz());
      }

      // Calculate maximum fragment charge
      int fragmentChargeMax = 1;
      if (!scan.getPrecursors().empty())
      {
         fragmentChargeMax = std::min(2, std::max(1, scan.getPrecursors()[0].getCharge() - 1));
      }

      // Set m/z limits
      double maxMz = scan.getEndMz();
      double minMz = scan.getStartMz();
      if (options_.getLowMassCutoff())
      {
         minMz = std::max(minMz, 0.28 * peptide.getPrecursorMz());
      }

      // Preprocess the scan
      PreProcessScan(scan);

      // Determine generator type and delegate to appropriate method
      bool useMultiMod = (options_.getSymbol() == '\0') || options_.getResidues().empty();

      ProcessPeptides(peptide, scan, fragmentChargeMax, minMz, maxMz, useMultiMod, output);

      return output;
   }

   void AScoreCalculator::PreProcessScan(Scan& scan)
   {
      // TODO: Add MatchDeisotoper to support Options.DeisotopingType == Deisotoping.MatchOffset
      // Apply deisotoping based on configured type
      if (options_.getDeisotopingType() == Deisotoping::Top1Per1)
      {
         TopIonsFilter topFilter(1, 1);
         topFilter.Filter(scan);
      }

      // Apply intensity filter to remove lowest intensity peaks
      IntensityFilter intensityFilter(options_.getFilterLowIntensity());
      intensityFilter.Filter(scan);

      // new WaterLossFilter(Options.Tolerance, Options.Units)
      WaterLossFilter waterLossFilter(options_.getTolerance(), options_.getUnits());
      waterLossFilter.Filter(scan);

      // new NeutralLossFilter(Options.Tolerance, Options.Units)
      NeutralLossFilter neutralLossFilter(options_.getTolerance(), options_.getUnits());
      neutralLossFilter.Filter(scan);

      // Apply top ions filter to keep only the most intense peaks per window
      TopIonsFilter topIonsFilter(options_.getMaxPeakDepth(), options_.getWindow());
      topIonsFilter.Filter(scan);
   }

   void AScoreCalculator::ProcessPeptides(
      const Peptide& peptide,
      const Scan& scan,
      int fragmentChargeMax,
      double minMz,
      double maxMz,
      bool useMultiMod,
      AScoreOutput& output)
   {
      std::unique_ptr<UnifiedPeptideGenerator> peptideGenerator;

      if (useMultiMod)
      {
         // Multi-modification mode: permute all modifications in peptide
         peptideGenerator = std::make_unique<UnifiedPeptideGenerator>(peptide, options_.getMasses());
      }
      else
      {
         // Single modification mode: permute only target modification
         PeptideMod targetMod;
         targetMod.setSymbol(options_.getSymbol());
         targetMod.setResidues(options_.getResidues());
         // Find mass from diff mods list
         for (const auto& mod : options_.getDiffMods())
         {
            if (mod.getSymbol() == targetMod.getSymbol())
            {
               targetMod.setMass(mod.getMass());
               break;
            }
         }
         peptideGenerator = std::make_unique<UnifiedPeptideGenerator>(peptide, targetMod, options_.getMasses());
      }

      // Set neutral loss if configured
      if (std::abs(options_.getNeutralLoss().getMass()) > 0 && !options_.getNeutralLoss().getResidues().empty())
      {
         peptideGenerator->setNeutralLossMod(options_.getNeutralLoss().getMass(), options_.getNeutralLoss().getResidues());
      }

      // Score generated peptides
      std::vector<Peptide> peptides;
      MOBScore scoring = MOBScore(options_);

      for (; !peptideGenerator->atEnd() && peptides.size() < static_cast<size_t>(options_.getMaxPeptides()); peptideGenerator->next())
      {
         auto ions = peptideGenerator->getMassList(options_.getIonSeries(), fragmentChargeMax, minMz, maxMz);
         Peptide p = peptideGenerator->getPeptide();
         p.setScore(scoring.score(p, ions, scan, output));
         peptides.push_back(p);
      }

      // Sort by score. Prefer original sequence if tied.
      struct PeptideScoreComparer
      {
         const Peptide& original;
         explicit PeptideScoreComparer(const Peptide& orig) : original(orig) {}
         bool operator()(const Peptide& a, const Peptide& b) const
         {
            if (a.getScore() == b.getScore())
            {
               return a.toString() == b.toString() ? 1 : 0;
            }
            return a.getScore() > b.getScore();
         }
      };

      std::sort(peptides.begin(), peptides.end(), PeptideScoreComparer(peptide));
      output.peptides = peptides;

      // Use best peak depth for site scoring (original score only)
      if (!peptides.empty())
      {
         Peptide& topPeptide = peptides[0];
         AScoreOptions siteOptions = options_;
         siteOptions.setPeakDepth(output.bestPeakDepth_);
         MOBScore siteScoring = MOBScore(options_);
         // auto siteScoring = ScoringFactory::Get(siteOptions);

         std::vector<SiteScore> sites;
         for (const auto& mod : topPeptide.getMods())
         {
            // Don't score other mods in target-mod mode
            if (!useMultiMod && mod.getSymbol() != options_.getSymbol())
            {
               continue;
            }

            // Default score is 5000 if there is only one possible
            // configuration for the current mod
            SiteScore siteOutput;
            siteOutput.setScore(5000.0);
            Peptide currentPeptide = topPeptide;

            // Find the next peptide that doesn't contain the mod at the position
            for (size_t i = 1; i < peptides.size(); ++i)
            {
               Peptide& next = peptides[i];
               bool hasModAtPosition = false;

               for (const auto& nextMod : next.getMods().getMods(mod.getPosition()))
               {
                  if (nextMod.getSymbol() == mod.getSymbol())   // TODO: Does it have to be the same symbol??
                  {
                     hasModAtPosition = true;
                     break;
                  }
               }

               if (hasModAtPosition)
               {
                  continue;
               }

               // Calculate site-determining ions
               peptideGenerator->setIndex(currentPeptide.getGeneratorIndex());
               auto ions1 = peptideGenerator->getMassList(options_.getIonSeries(), fragmentChargeMax, minMz, maxMz);

               peptideGenerator->setIndex(next.getGeneratorIndex());
               auto ions2 = peptideGenerator->getMassList(options_.getIonSeries(), fragmentChargeMax, minMz, maxMz);

               // Score the site determining ions
               auto siteIons = SiteIons::Filter(ions1, ions2);
               siteOutput.setScore(siteScoring.score(currentPeptide, siteIons, scan, output));

               if (siteOutput.getPeptides().size() < 1)
               {
                  siteOutput.getPeptides().push_back(currentPeptide.clone());
               }
               if (siteOutput.getSiteIons().size() < 1)
               {
                  siteOutput.getSiteIons().push_back(std::vector<Centroid>(siteIons));
               }

               if (options_.getUseDeltaAscore())
               {
                  // Flip order and get the score difference
                  auto siteIons2 = SiteIons::Filter(ions2, ions1);
                  double nextScore = siteScoring.score(next, siteIons2, scan, output);
                  siteOutput.setScore(siteOutput.getScore() - nextScore);
               }

               siteOutput.getPeptides().push_back(next);

               // Score only once per mod
               break;
            }

            // Convert to 1-based position
            siteOutput.setPosition(mod.getPosition() + 1);
            sites.push_back(siteOutput);
         }

         output.sites = sites;
      }
   }

} // namespace AScoreProCpp