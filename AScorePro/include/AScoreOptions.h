#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "API.h"
#include "AminoAcidMasses.h"
#include "Deisotoping.h"
#include "Mass.h"
#include "NeutralLoss.h"
#include "PeptideMod.h"

namespace AScoreProCpp {

    /**
     * Options for the AScore algorithm.
     */
    class ASCORE_API AScoreOptions {
    public:
        // Default constructor
        AScoreOptions();

        // Copy constructor
        AScoreOptions(const AScoreOptions& other);

        // Assignment operator
        AScoreOptions& operator=(const AScoreOptions& other);

        // Getters and setters for each property

        const std::string& getScans() const;
        void setScans(const std::string& scans);

        const std::vector<std::string>& getIonSeriesList() const;
        void setIonSeriesList(const std::vector<std::string>& ionSeriesList);

        int getIonSeries() const;
        void setIonSeries(int ionSeries);

        const std::vector<PeptideMod>& getDiffMods() const;
        void setDiffMods(const std::vector<PeptideMod>& diffMods);

        const std::vector<PeptideMod>& getStaticMods() const;
        void setStaticMods(const std::vector<PeptideMod>& staticMods);

        const NeutralLoss& getNeutralLoss() const;
        void setNeutralLoss(const NeutralLoss& neutralLoss);

        int getPeakDepth() const;
        void setPeakDepth(int peakDepth);

        int getMaxPeakDepth() const;
        void setMaxPeakDepth(int maxPeakDepth);

        double getTolerance() const;
        void setTolerance(double tolerance);

        const std::string& getUnitText() const;
        void setUnitText(const std::string& unitText);

        Mass::Units getUnits() const;
        void setUnits(Mass::Units units);

        int getWindow() const;
        void setWindow(int window);

        bool getLowMassCutoff() const;
        void setLowMassCutoff(bool lowMassCutoff);

        double getFilterLowIntensity() const;
        void setFilterLowIntensity(double filterLowIntensity);

        Deisotoping getDeisotopingType() const;
        void setDeisotopingType(Deisotoping deisotopingType);

        bool getNoCterm() const;
        void setNoCterm(bool noCterm);

        bool getUseMobScore() const;
        void setUseMobScore(bool useMobScore);

        bool getUseDeltaAscore() const;
        void setUseDeltaAscore(bool useDeltaAscore);

        char getSymbol() const;
        void setSymbol(char symbol);

        const std::string& getResidues() const;
        void setResidues(const std::string& residues);

        const std::string& getOut() const;
        void setOut(const std::string& out);

        int getMaxPeptides() const;
        void setMaxPeptides(int maxPeptides);

        int getMz() const;
        void setMz(int mz);

        const std::string& getPeptide() const;
        void setPeptide(const std::string& peptide);

        int getScan() const;
        void setScan(int scan);

        const std::string& getPeptidesFile() const;
        void setPeptidesFile(const std::string& peptidesFile);

        AminoAcidMasses& getMasses();
        const AminoAcidMasses& getMasses() const;
        void setMasses(const AminoAcidMasses& masses);

    private:
        // The name of the file containing the scan information
        std::string scans_;

        // List of fragment ion series to consider
        std::vector<std::string> ionSeriesList_;

        // Stores the unserialized ion series data as flags in this integer
        int ionSeries_;

        // List of Diff mod information
        std::vector<PeptideMod> diffMods_;

        // List of static mod information
        std::vector<PeptideMod> staticMods_;

        // Options for modification that can generate neutral loss fragments
        NeutralLoss neutralLoss_;

        // During scoring, the scan is filtered to a peak depth per m/z window
        int peakDepth_;

        // Peak depths up to this value are used when scoring
        int maxPeakDepth_;

        // Peak match tolerance. Used with Units option
        double tolerance_;

        // Text read from the Unit Option
        std::string unitText_;

        // Stores parsed unit selection from the option read into UnitText
        Mass::Units units_;

        // The m/z window size to use when filtering and scoring peaks in scan
        int window_;

        // When enabled, ions below 0.28 of the max m/z are not considered
        bool lowMassCutoff_;

        // Scans are filtered to remove this fraction of its lowest intensity peaks
        double filterLowIntensity_;

        // Type of deisotoping to apply
        Deisotoping deisotopingType_;

        // When enabled, modifications on the c-terminus of the peptide are not considered
        bool noCterm_;

        // Enable to use MOB Scoring algorithm instead of Original AScore algorithm
        bool useMobScore_;

        // If true, the score is subtracted by the site score of the top two peptides when they are reversed
        bool useDeltaAscore_;

        // The symbol of the modification to score
        char symbol_;

        // The residues to consider for the modification being scored
        std::string residues_;

        // Output file path
        std::string out_;

        // Max mod combinations to consider per peptide
        int maxPeptides_;

        // When scoring a single peptide, this is the precursor m/z of the peptide
        int mz_;

        // When scoring a single peptide, this stores the annotated sequence
        std::string peptide_;

        // When scoring a single peptide, this is the scan number
        int scan_;

        // Path to the csv file containing the input list of peptides to score
        std::string peptidesFile_;

        // Instance of amino acid masses with static mods applied
        AminoAcidMasses masses_;
    };

} // namespace AScoreProCpp