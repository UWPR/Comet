#pragma once

#include <vector>
#include <map>
#include <string>
#include "AScorePeptide.h"
#include "AScorePeptideMod.h"
#include "AScoreAminoAcidMasses.h"
#include "AScoreCentroid.h"
#include "AScoreMass.h"
#include "AScoreCombinationIterator.h"

namespace AScoreProCpp {

    /**
     * Represents a modification type
     */
    struct ModificationType {
        char symbol;                  // e.g., '#' for phosphorylation, '*' for oxidation
        std::string residues;         // e.g., "STY", "M"
        double mass;                  // mass difference
        int count;                    // number of residues in the input peptide with this modification

        ModificationType();
        ModificationType(char sym, const std::string& res, double m, int c);

        /**
         * Check if this modification can apply to a residue
         */
        bool applies(char residue) const;

        /**
         * Convert to PeptideMod for compatibility
         */
        PeptideMod toPeptideMod() const;
    };

    /**
     * Unified peptide generator that handles both single (target) and multiple modification types.
     *
     * This class can operate in two modes:
     * 1. Target modification mode: Permutes only one target modification type
     * 2. Multi modification mode: Permutes all modification types from input peptide
     */
    class UnifiedPeptideGenerator {
private:
        /**
         * Represents a particular arrangement of modifications.
         */
        struct Configuration {
            std::map<int, ModificationType> positionToMod;
            int generatorIndex;

            Configuration() : generatorIndex(-1) {}

            void setModification(int position, const ModificationType& modType) {
                positionToMod[position] = modType;
            }

            const ModificationType* getModAtPosition(int position) const {
                auto it = positionToMod.find(position);
                return (it != positionToMod.end()) ? &(it->second) : nullptr;
            }

            const std::map<int, ModificationType>& getAllMods() const {
                return positionToMod;
            }

            std::string toString() const {
                std::string result = "{";
                bool first = true;
                for (const auto& pair : positionToMod) {
                    if (!first) result += ", ";
                    result += pair.second.symbol;
                    result += "@" + std::to_string(pair.first + 1);
                    first = false;
                }
                result += "}";
                return result;
            }

            Peptide toPeptide(const Peptide& basePeptide) const {
                Peptide result = basePeptide.clone();
                result.setGeneratorIndex(generatorIndex);

                // Start with existing modifications from basePeptide
                PeptideMods newMods = basePeptide.getMods();

                // Add the target modifications from this configuration
                for (const auto& pair : positionToMod) {
                    PeptideMod peptideMod = pair.second.toPeptideMod();
                    newMods.add(pair.first, peptideMod);
                }

                result.setMods(newMods);
                return result;
            }
        };

public:
        /**
         * Constructor for single (target) modification mode
         * Generates all possible positions for a target modification
         *
         * @param peptide Base peptide (may contain other non-target modifications)
         * @param targetMod The modification to permute across valid positions
         * @param aaMasses Amino acid mass reference
         */
        UnifiedPeptideGenerator(const Peptide& peptide,
            const PeptideMod& targetMod,
            const AminoAcidMasses& aaMasses);

        /**
         * Constructor for multi-modification mode
         * Generates all possible rearrangements of all modifications in input peptide
         *
         * @param inputPeptide Peptide with modifications to permute
         * @param aaMasses Amino acid mass reference
         */
        UnifiedPeptideGenerator(const Peptide& inputPeptide,
            const AminoAcidMasses& aaMasses);

        /**
         * Check if all configurations have been enumerated
         */
        bool atEnd() const;

        /**
         * Advance to next configuration
         */
        void next();

        /**
         * Set current configuration index
         */
        void setIndex(int index);

        /**
         * Get current configuration index
         */
        int getCurrentIndex() const;

        /**
         * Get total number of possible configurations
         */
        size_t getTotalConfigurations() const;

        /**
         * Set neutral loss modification parameters
         */
        void setNeutralLossMod(double mass, const std::string& residues);

        /**
         * Get peptide with current modification arrangement
         */
        Peptide getPeptide();

        /**
         * Get theoretical fragment ions for current configuration
         *
         * @param ionSeriesFlags Bitwise OR of Mass::IonSeries values
         * @param maxCharge Maximum fragment charge state
         * @param minMz Minimum m/z for fragments
         * @param maxMz Maximum m/z for fragments
         * @return Theoretical fragment ions sorted by m/z
         */
        std::vector<Centroid> getMassList(int ionSeriesFlags, int maxCharge,
            double minMz, double maxMz);

        /**
         * Get modification types being permuted
         */
        std::vector<ModificationType> getModificationTypes() const;

        /**
         * Debug: Print all configurations to console
         */
        void printAllConfigurations() const;

private:
        /**
         * Initialize for target modification mode
         */
        void initTargetMod(const Peptide& peptide, const PeptideMod& targetMod);

        /**
         * Initialize for multi-modification mode
         */
        void initMultiMod(const Peptide& inputPeptide);

        /**
         * Extract modification types from input peptide
         */
        std::vector<ModificationType> extractModificationTypes(const Peptide& inputPeptide);

        /**
         * Initialize base masses (AA + terminals, no target modifications)
         */
        void initializeBaseMasses();

        /**
         * Generate all configurations recursively
         */
        void generateAllConfigurations();

        /**
         * Recursively generate configurations for each modification type
         */
        void generateConfigurationsRecursive(int modTypeIndex, Configuration currentConfig);

        /**
         * Calculate ion m/z from mass and charge
         */
        double ionMz(double mass, int charge) const;

        /**
         * Insert an ion m/z into a list if it falls within a range
         */
        void insertInRange(std::vector<Centroid>& ions, double minMz, double maxMz, double mz);

        // Member variables
        Peptide basePeptide_;                      // Base peptide without target mods
        std::vector<ModificationType> modTypes_;   // Modification types to permute
        AminoAcidMasses masses_;                   // Amino acid masses

        // Neutral loss support
        double neutralLossMass_;
        std::string neutralLossResidues_;
        bool useNeutralLoss_;

        // Configuration management
        std::vector<Configuration> configurations_; // All possible configurations
        int currentIndex_;                         // Current configuration index
        int maxPeptides_;                          // Maximum configurations to generate

        // Mass calculations
        std::vector<double> baseMasses_;           // Base masses (AA + terminals)
    };

} // namespace AScoreProCpp
