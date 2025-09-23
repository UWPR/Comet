#include <algorithm>
#include <iostream>
#include <cmath>
#include "AScorePeptideGenerator.h"

namespace AScoreProCpp
{

   PeptideGenerator::PeptideGenerator(const Peptide& peptide, const PeptideMod& targetMod, const AminoAcidMasses& aaMasses) :
      neutralLossMass_(0.0),
      neutralLossResidues_(""),
      useNeutralLoss_(false),
      basePeptide_(peptide.clone()),
      index_(0),
      maxPeptides_(1000),
      masses_(aaMasses.clone()),
      targetMod_(targetMod.clone())
   {
      init();
   }

   void PeptideGenerator::init()
   {
      // Count number of target mod.
      // Remove existing target mods.
      int modCount = 0;
      PeptideMods mods;
      for (const auto& mod : basePeptide_.getMods())
      {
         if (mod.getSymbol() == targetMod_.getSymbol())
         {
            ++modCount;
         }
         else
         {
            mods.add(mod.getPosition(), mod);
         }
      }
      basePeptide_.setMods(mods);

      // Find possible positions.
      const std::string& sequence = basePeptide_.getSequence();
      std::vector<int> modPositions;

      for (int i = 0; i < static_cast<int>(sequence.length()); ++i)
      {
         if (targetMod_.applies(sequence[i]))
         {
            modPositions.push_back(i);
         }
      }

      // Generate combinations of modification positions
      CombinationIterator generator;
      combinations_.clear();

      // The template type must be int to match the modPositions vector
      std::function<void(const std::vector<int>&)> callback =
         [this](const std::vector<int>& combination)
      {
         // Check if we're still under the maximum before adding
         if (combinations_.size() < static_cast<size_t>(maxPeptides_))
         {
            combinations_.push_back(combination);
         }
      };

      // Call Generate with the properly typed callback
      generator.Generate<int>(modPositions, modCount, callback);

      // Limit the number of combinations if necessary
      if (combinations_.size() > static_cast<size_t>(maxPeptides_))
      {
         combinations_.resize(maxPeptides_);
      }

      index_ = 0;

      // Initialize mass list, including non-target mods.
      modMasses_.resize(sequence.length());
      aaMasses_.resize(sequence.length());

      for (size_t i = 0; i < sequence.length(); ++i)
      {
         char aa = sequence[i];
         double mass = masses_.getAminoAcidMass(aa);
         modMasses_[i] = mass;
         aaMasses_[i] = mass;
      }

      // Add terminal masses
      modMasses_[0] += masses_.getNTermMass();
      modMasses_[modMasses_.size() - 1] += masses_.getCTermMass();

      // Copy to aaMasses_
      aaMasses_[0] += masses_.getNTermMass();
      aaMasses_[aaMasses_.size() - 1] += masses_.getCTermMass();

      // Apply existing mods
      const PeptideMods& existingMods = basePeptide_.getMods();
      if (!existingMods.empty())
      {
         for (const auto& mod : existingMods)
         {
            // Check if the position is valid before applying
            if (mod.getPosition() >= 0 && mod.getPosition() < static_cast<int>(modMasses_.size()))
            {
               modMasses_[mod.getPosition()] += mod.getMass();
               aaMasses_[mod.getPosition()] += mod.getMass();
            }
            else
            {
               // Handle error: mod position is out of bounds
               std::cerr << "Warning: Modification position " << mod.getPosition()
                         << " is out of bounds for peptide sequence of length "
                         << modMasses_.size() << std::endl;
            }
         }
      }
   }

   bool PeptideGenerator::atEnd() const
   {
      return index_ >= static_cast<int>(combinations_.size());
   }

   void PeptideGenerator::next()
   {
      ++index_;
   }

   void PeptideGenerator::setNeutralLossMod(double mass, const std::string& residues)
   {
      neutralLossMass_ = mass;
      neutralLossResidues_ = residues;
      useNeutralLoss_ = true;
   }

   void PeptideGenerator::setIndex(int i)
   {
      index_ = i;
   }

   std::vector<Centroid> PeptideGenerator::getMassList(int ionSeriesFlags, int maxCharge, double minMz, double maxMz)
   {
      // Reset modification masses to initial values
      for (size_t i = 0; i < modMasses_.size(); ++i)
      {
         modMasses_[i] = aaMasses_[i];
      }

      // Apply target modifications at current combination positions
      for (int pos : combinations_[index_])
      {
         modMasses_[pos] += targetMod_.getMass();
      }

      const std::string& sequence = basePeptide_.getSequence();

      // Keep track of positions that can produce neutral loss fragments
      std::vector<bool> posCanNL(modMasses_.size(), false);

      if (useNeutralLoss_)
      {
         for (int pos : combinations_[index_])
         {
            for (char nlRes : neutralLossResidues_)
            {
               if (sequence[pos] == nlRes)
               {
                  posCanNL[pos] = true;
                  break;
               }
            }
         }
      }

      // Calculate a, b, and c ion series
      double aMass = 0.0;
      double bMass = Mass::Proton;
      double cMass = Mass::Nitrogen + (3 * Mass::Hydrogen) - Mass::Electron;

      std::vector<Centroid> output;
      bool fragCanNL = false;

      for (size_t i = 0; i < modMasses_.size(); ++i)
      {
         double residueMass = modMasses_[i];
         bMass += residueMass;
         cMass += residueMass;
         aMass = bMass - Mass::Carbon - Mass::Oxygen;

         fragCanNL |= posCanNL[i];

         for (int j = 1; j <= maxCharge; ++j)
         {
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::A_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(aMass, j));
            }
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::B_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(bMass, j));
            }
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::C_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(cMass, j));
            }

            // Fragments after neutral loss
            if (useNeutralLoss_ && fragCanNL)
            {
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::A_NEUTRAL_LOSS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(aMass + neutralLossMass_, j));
               }
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::B_NEUTRAL_LOSS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(bMass + neutralLossMass_, j));
               }
               // There's no C_NEUTRAL_LOSS in the enum, so we'll check for C_IONS
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::C_IONS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(cMass + neutralLossMass_, j));
               }
            }
         }
      }

      // Calculate x, y, and z ion series
      fragCanNL = false;
      double xMass = 0.0;
      double yMass = (3 * Mass::Hydrogen) + Mass::Oxygen - Mass::Electron;
      double zMass = 2.99966565 - Mass::Electron;

      for (int i = static_cast<int>(sequence.length()) - 1; i > 0; --i)
      {
         double residueMass = modMasses_[i];
         yMass += residueMass;
         zMass += residueMass;
         xMass = yMass - Mass::Carbon - Mass::Oxygen;

         fragCanNL |= posCanNL[i];

         for (int j = 1; j <= maxCharge; ++j)
         {
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::X_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(xMass, j));
            }
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::Y_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(yMass, j));
            }
            if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::Z_IONS))
            {
               insertInRange(output, minMz, maxMz, ionMz(zMass, j));
            }

            if (useNeutralLoss_ && fragCanNL)
            {
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::X_IONS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(xMass + neutralLossMass_, j));
               }
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::Y_NEUTRAL_LOSS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(yMass + neutralLossMass_, j));
               }
               if (ionSeriesFlags & static_cast<int>(Mass::IonSeries::Z_IONS))
               {
                  insertInRange(output, minMz, maxMz, ionMz(zMass + neutralLossMass_, j));
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

   Peptide PeptideGenerator::getPeptide()
   {
      Peptide output = basePeptide_.clone();
      output.setAnnotatedSequence("");
      output.setGeneratorIndex(index_);

      for (auto it = combinations_[index_].rbegin(); it != combinations_[index_].rend(); ++it)
      {
         output.getMods().add(*it, targetMod_);
      }

      return output;
   }

   double PeptideGenerator::ionMz(double mass, int charge) const
   {
      return (mass + ((charge - 1) * Mass::Proton)) / charge;
   }

   void PeptideGenerator::insertInRange(std::vector<Centroid>& ions, double minMz, double maxMz, double mz)
   {
      // TODO: Consider adding a tolerance for theoretical ions that fall very close to minMz or maxMz
      if (mz > minMz && mz < maxMz)
      {
         // Using the constructor from the provided Centroid class
         Centroid centroid(mz, 1.0);
         ions.push_back(centroid);
      }
   }

} // namespace AScoreProCpp
