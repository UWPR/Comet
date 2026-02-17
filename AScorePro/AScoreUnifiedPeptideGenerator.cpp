#include "AScoreUnifiedPeptideGenerator.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <functional>

namespace AScoreProCpp {

   // =================== ModificationType Implementation ===================

   ModificationType::ModificationType() : symbol('\0'), mass(0.0), count(0) {}

   ModificationType::ModificationType(char sym, const std::string& res, double m, int c)
      : symbol(sym), residues(res), mass(m), count(c)
   {
   }

   bool ModificationType::applies(char residue) const
   {
      return residues.find(residue) != std::string::npos;
   }

   PeptideMod ModificationType::toPeptideMod() const
   {
      PeptideMod mod;
      mod.setSymbol(symbol);
      mod.setResidues(residues);
      mod.setMass(mass);
      return mod;
   }

   // =================== UnifiedPeptideGenerator Implementation ===================

   UnifiedPeptideGenerator::UnifiedPeptideGenerator(const Peptide& peptide,
      const PeptideMod& targetMod,
      const AminoAcidMasses& aaMasses)
      : masses_(aaMasses.clone()),
      neutralLossMass_(0.0),
      neutralLossResidues_(""),
      useNeutralLoss_(false),
      currentIndex_(0),
      maxPeptides_(1000)
   {
      basePeptide_ = peptide.clone();
      initTargetMod(peptide, targetMod);
   }

   UnifiedPeptideGenerator::UnifiedPeptideGenerator(const Peptide& inputPeptide,
      const AminoAcidMasses& aaMasses)
      : masses_(aaMasses.clone()),
      neutralLossMass_(0.0),
      neutralLossResidues_(""),
      useNeutralLoss_(false),
      currentIndex_(0),
      maxPeptides_(1000)
   {
      basePeptide_ = inputPeptide.clone();
      initMultiMod(inputPeptide);
   }

   void UnifiedPeptideGenerator::initTargetMod(const Peptide& peptide, const PeptideMod& targetMod)
   {
      // Count target modifications and remove them from base peptide
      int modCount = 0;
      PeptideMods nonTargetMods;

      for (const auto& mod : peptide.getMods())
      {
         if (mod.getSymbol() == targetMod.getSymbol())
            ++modCount;
         else
            nonTargetMods.add(mod.getPosition(), mod);
      }

      basePeptide_.setMods(nonTargetMods);

      // Create target modification type
      ModificationType targetModType(
         targetMod.getSymbol(),
         targetMod.getResidues(),
         targetMod.getMass(),
         modCount
      );

      modTypes_.push_back(targetModType);

      // Initialize and generate
      initializeBaseMasses();
      generateAllConfigurations();

      // Apply max peptides limit
      if (configurations_.size() > static_cast<size_t>(maxPeptides_))
      {
         configurations_.resize(maxPeptides_);
      }

      currentIndex_ = 0;
   }

   void UnifiedPeptideGenerator::initMultiMod(const Peptide& inputPeptide)
   {
      // Remove all mods from base peptide
      basePeptide_.setMods(PeptideMods());

      // Extract modification types
      modTypes_ = extractModificationTypes(inputPeptide);

      // Initialize and generate
      initializeBaseMasses();
      generateAllConfigurations();
      currentIndex_ = 0;
   }

   std::vector<ModificationType> UnifiedPeptideGenerator::extractModificationTypes(const Peptide& inputPeptide)
   {
      std::map<char, ModificationType> modTypeMap;

      const PeptideMods& mods = inputPeptide.getMods();
      if (mods.empty())
         return std::vector<ModificationType>();

      for (const auto& mod : mods)
      {
         char symbol = mod.getSymbol();

         if (modTypeMap.find(symbol) == modTypeMap.end())
         {
            ModificationType modType(symbol, mod.getResidues(), mod.getMass(), 1);
            modTypeMap[symbol] = modType;
         }
         else
         {
            modTypeMap[symbol].count++;
         }
      }

      std::vector<ModificationType> result;
      for (const auto& pair : modTypeMap)
         result.push_back(pair.second);

      return result;
   }

   void UnifiedPeptideGenerator::initializeBaseMasses()
   {
      const std::string& sequence = basePeptide_.getSequence();
      baseMasses_.resize(sequence.length());

      for (size_t i = 0; i < sequence.length(); ++i)
      {
         char aa = sequence[i];
         baseMasses_[i] = masses_.getAminoAcidMass(aa);
      }

      // Add terminal masses
      if (!baseMasses_.empty())
      {
         baseMasses_[0] += masses_.getNTermMass();
         baseMasses_[baseMasses_.size() - 1] += masses_.getCTermMass();
      }

      // Apply non-target modifications from base peptide
      const PeptideMods& existingMods = basePeptide_.getMods();
      if (!existingMods.empty())
      {
         for (const auto& mod : existingMods)
         {
            if (mod.getPosition() >= 0 && mod.getPosition() < static_cast<int>(baseMasses_.size()))
            {
               baseMasses_[mod.getPosition()] += mod.getMass();
            }
            else
            {
               std::cerr << "Warning: Modification position " << mod.getPosition()
                  << " is out of bounds for peptide sequence of length "
                  << baseMasses_.size() << std::endl;
            }
         }
      }
   }

   void UnifiedPeptideGenerator::generateAllConfigurations()
   {
      configurations_.clear();
      generateConfigurationsRecursive(0, Configuration());

      // Set generator indices
      for (size_t i = 0; i < configurations_.size(); ++i)
         configurations_[i].generatorIndex = static_cast<int>(i);
   }

   void UnifiedPeptideGenerator::generateConfigurationsRecursive(int modTypeIndex, Configuration currentConfig)
   {
      if (modTypeIndex >= static_cast<int>(modTypes_.size()))
      {
         // Base case: all modification types processed
         configurations_.push_back(currentConfig);
         return;
      }

      const ModificationType& currentModType = modTypes_[modTypeIndex];
      const std::string& sequence = basePeptide_.getSequence();

      // Find valid positions for this modification type
      std::vector<int> validPositions;
      for (int pos = 0; pos < static_cast<int>(sequence.length()); ++pos)
      {
         if (currentModType.applies(sequence[pos]))
         {
            if (currentConfig.getModAtPosition(pos) == nullptr)
            {
               validPositions.push_back(pos);
            }
         }
      }

      // Use CombinationIterator to generate all placement combinations
      CombinationIterator combIterator;

      std::function<void(const std::vector<int>&)> callback =
         [this, currentConfig, &currentModType, modTypeIndex](const std::vector<int>& combination)
         {
            Configuration newConfig = currentConfig;

            for (int pos : combination)
               newConfig.setModification(pos, currentModType);

            generateConfigurationsRecursive(modTypeIndex + 1, newConfig);
         };

      combIterator.Generate<int>(validPositions, currentModType.count, callback);
   }

   bool UnifiedPeptideGenerator::atEnd() const
   {
      return currentIndex_ >= static_cast<int>(configurations_.size());
   }

   void UnifiedPeptideGenerator::next()
   {
      ++currentIndex_;
   }

   void UnifiedPeptideGenerator::setIndex(int index)
   {
      currentIndex_ = index;
   }

   int UnifiedPeptideGenerator::getCurrentIndex() const
   {
      return currentIndex_;
   }

   size_t UnifiedPeptideGenerator::getTotalConfigurations() const
   {
      return configurations_.size();
   }

   void UnifiedPeptideGenerator::setNeutralLossMod(double mass, const std::string& residues)
   {
      neutralLossMass_ = mass;
      neutralLossResidues_ = residues;
      useNeutralLoss_ = true;
   }

   Peptide UnifiedPeptideGenerator::getPeptide()
   {
      if (atEnd())
         throw std::out_of_range("Generator is at end");

      return configurations_[currentIndex_].toPeptide(basePeptide_);
   }

   std::vector<ModificationType> UnifiedPeptideGenerator::getModificationTypes() const
   {
      return modTypes_;
   }

   std::vector<Centroid> UnifiedPeptideGenerator::getMassList(int ionSeriesFlags,
      int maxCharge,
      double minMz,
      double maxMz)
   {
      if (atEnd())
         throw std::out_of_range("Generator is at end");

      const Configuration& config = configurations_[currentIndex_];
      const std::string& sequence = basePeptide_.getSequence();

      // Calculate effective masses for current configuration
      std::vector<double> effectiveMasses = baseMasses_;

      for (const auto& pair : config.getAllMods())
      {
         int position = pair.first;
         const ModificationType& modType = pair.second;

         if (position >= 0 && position < static_cast<int>(effectiveMasses.size()))
            effectiveMasses[position] += modType.mass;
      }

      // Track positions for neutral loss
      std::vector<bool> posCanNL(effectiveMasses.size(), false);
      if (useNeutralLoss_)
      {
         for (const auto& pair : config.getAllMods())
         {
            int position = pair.first;
            if (position >= 0 && position < static_cast<int>(sequence.length()))
            {
               char residue = sequence[position];
               if (neutralLossResidues_.find(residue) != std::string::npos)
                  posCanNL[position] = true;
            }
         }
      }

      std::vector<Centroid> output;

      // Generate N-terminal ions (a, b, c)
      double aMass = 0.0;
      double bMass = Mass::Proton;
      double cMass = Mass::Nitrogen + (3 * Mass::Hydrogen) - Mass::Electron;
      bool fragCanNL = false;

      for (size_t i = 0; i < effectiveMasses.size(); ++i)
      {
         double residueMass = effectiveMasses[i];
         bMass += residueMass;
         cMass += residueMass;
         aMass = bMass - Mass::Carbon - Mass::Oxygen;

         fragCanNL |= posCanNL[i];

         for (int charge = 1; charge <= maxCharge; ++charge)
         {
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::A_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(aMass, charge));
            }
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::B_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(bMass, charge));
            }
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::C_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(cMass, charge));
            }

            // Fragments after neutral loss
            if (useNeutralLoss_ && fragCanNL) {
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::A_NEUTRAL_LOSS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(aMass + neutralLossMass_, charge));
               }
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::B_NEUTRAL_LOSS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(bMass + neutralLossMass_, charge));
               }
               // There's no C_NEUTRAL_LOSS in the enum, so we'll check for C_IONS
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::C_IONS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(cMass + neutralLossMass_, charge));
               }
            }
         }
      }

      // Generate C-terminal ions (x, y, z)
      fragCanNL = false;
      double xMass = 0.0;
      double yMass = (3 * Mass::Hydrogen) + Mass::Oxygen - Mass::Electron;
      double zMass = 2.99966565 - Mass::Electron;

      for (int i = static_cast<int>(sequence.length()) - 1; i > 0; --i)
      {
         double residueMass = effectiveMasses[i];
         yMass += residueMass;
         zMass += residueMass;
         xMass = yMass - Mass::Carbon - Mass::Oxygen;

         fragCanNL |= posCanNL[i];

         for (int charge = 1; charge <= maxCharge; ++charge)
         {
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::X_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(xMass, charge));
            }
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::Y_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(yMass, charge));
            }
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::Z_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(zMass, charge));
            }

            if (useNeutralLoss_ && fragCanNL)
            {
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::X_IONS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(xMass + neutralLossMass_, charge));
               }
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::Y_NEUTRAL_LOSS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(yMass + neutralLossMass_, charge));
               }
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::Z_IONS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(zMass + neutralLossMass_, charge));
               }
            }
         }
      }

      // Sort output by m/z
      std::sort(output.begin(), output.end(), [](const Centroid& a, const Centroid& b)
         {
            return a.getMz() < b.getMz();
         });

      return output;
   }

   void UnifiedPeptideGenerator::printAllConfigurations() const
   {
      std::cout << "All " << configurations_.size() << " configurations:" << std::endl;

      for (size_t i = 0; i < configurations_.size(); ++i)
         std::cout << "  " << (i + 1) << ": " << configurations_[i].toString() << std::endl;
   }

   double UnifiedPeptideGenerator::ionMz(double mass, int charge) const
   {
      return (mass + ((charge - 1) * Mass::Proton)) / charge;
   }

   void UnifiedPeptideGenerator::insertInRange(std::vector<Centroid>& ions, double minMz, double maxMz, double mz)
   {
      if (mz > minMz && mz < maxMz)
         ions.emplace_back(mz, 1.0);
   }

} // namespace AScoreProCpp