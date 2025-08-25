#include "AScoreOptions.h"

namespace AScoreProCpp
{

AScoreOptions::AScoreOptions() :
   ionSeries_(0),
   peakDepth_(0),
   maxPeakDepth_(0),
   tolerance_(0.0),
   units_(Mass::Units::DALTON),
   window_(0),
   lowMassCutoff_(false),
   filterLowIntensity_(0.25),
   deisotopingType_(Deisotoping::None),
   noCterm_(false),
   useMobScore_(false),
   useDeltaAscore_(false),
   symbol_('\0'),
   maxPeptides_(0),
   mz_(0),
   scan_(0)
{
   // Default constructor implementation
}

AScoreOptions::AScoreOptions(const AScoreOptions& other) :
   scans_(other.scans_),
   ionSeriesList_(other.ionSeriesList_),
   ionSeries_(other.ionSeries_),
   diffMods_(other.diffMods_),
   staticMods_(other.staticMods_),
   neutralLoss_(other.neutralLoss_),
   peakDepth_(other.peakDepth_),
   maxPeakDepth_(other.maxPeakDepth_),
   tolerance_(other.tolerance_),
   unitText_(other.unitText_),
   units_(other.units_),
   window_(other.window_),
   lowMassCutoff_(other.lowMassCutoff_),
   filterLowIntensity_(other.filterLowIntensity_),
   deisotopingType_(other.deisotopingType_),
   noCterm_(other.noCterm_),
   useMobScore_(other.useMobScore_),
   useDeltaAscore_(other.useDeltaAscore_),
   symbol_(other.symbol_),
   residues_(other.residues_),
   out_(other.out_),
   maxPeptides_(other.maxPeptides_),
   mz_(other.mz_),
   peptide_(other.peptide_),
   scan_(other.scan_),
   peptidesFile_(other.peptidesFile_),
   masses_(other.masses_.clone())
{
   // Copy constructor implementation
}

AScoreOptions& AScoreOptions::operator=(const AScoreOptions& other)
{
   if (this != &other)
   {
      scans_ = other.scans_;
      ionSeriesList_ = other.ionSeriesList_;
      ionSeries_ = other.ionSeries_;
      diffMods_ = other.diffMods_;
      staticMods_ = other.staticMods_;
      neutralLoss_ = other.neutralLoss_;
      peakDepth_ = other.peakDepth_;
      maxPeakDepth_ = other.maxPeakDepth_;
      tolerance_ = other.tolerance_;
      unitText_ = other.unitText_;
      units_ = other.units_;
      window_ = other.window_;
      lowMassCutoff_ = other.lowMassCutoff_;
      filterLowIntensity_ = other.filterLowIntensity_;
      deisotopingType_ = other.deisotopingType_;
      noCterm_ = other.noCterm_;
      useMobScore_ = other.useMobScore_;
      useDeltaAscore_ = other.useDeltaAscore_;
      symbol_ = other.symbol_;
      residues_ = other.residues_;
      out_ = other.out_;
      maxPeptides_ = other.maxPeptides_;
      mz_ = other.mz_;
      peptide_ = other.peptide_;
      scan_ = other.scan_;
      peptidesFile_ = other.peptidesFile_;
      masses_ = other.masses_.clone();
   }
   return *this;
}

// Getters and setters implementation
const std::string& AScoreOptions::getScans() const
{
   return scans_;
}

void AScoreOptions::setScans(const std::string& scans)
{
   scans_ = scans;
}

const std::vector<std::string>& AScoreOptions::getIonSeriesList() const
{
   return ionSeriesList_;
}

void AScoreOptions::setIonSeriesList(const std::vector<std::string>& ionSeriesList)
{
   ionSeriesList_ = ionSeriesList;

   // Map ion series names to their bit flags
   std::unordered_map<std::string, int> ionSeriesOptions =
   {
      { "nA", 1 },
      { "nB", 2 },
      { "nY", 4 },
      { "a", 8 },
      { "b", 16 },
      { "c", 32 },
      { "d", 64 },
      { "v", 128 },
      { "w", 256 },
      { "x", 512 },
      { "y", 1024 },
      { "z", 2048 }
   };

   int ionSeries = 0;
   for (const auto& series : ionSeriesList)
   {
      auto it = ionSeriesOptions.find(series);
      if (it != ionSeriesOptions.end())
      {
         ionSeries += it->second;
      }
   }
   setIonSeries(ionSeries);
}

int AScoreOptions::getIonSeries() const
{
   return ionSeries_;
}

void AScoreOptions::setIonSeries(int ionSeries)
{
   ionSeries_ = ionSeries;
}

const std::vector<PeptideMod>& AScoreOptions::getDiffMods() const
{
   return diffMods_;
}

void AScoreOptions::setDiffMods(const std::vector<PeptideMod>& diffMods)
{
   diffMods_ = diffMods;
}

const std::vector<PeptideMod>& AScoreOptions::getStaticMods() const
{
   return staticMods_;
}

void AScoreOptions::setStaticMods(const std::vector<PeptideMod>& staticMods)
{
   staticMods_ = staticMods;
}

const NeutralLoss& AScoreOptions::getNeutralLoss() const
{
   return neutralLoss_;
}

void AScoreOptions::setNeutralLoss(const NeutralLoss& neutralLoss)
{
   neutralLoss_ = neutralLoss;
}

int AScoreOptions::getPeakDepth() const
{
   return peakDepth_;
}

void AScoreOptions::setPeakDepth(int peakDepth)
{
   peakDepth_ = peakDepth;
}

int AScoreOptions::getMaxPeakDepth() const
{
   return maxPeakDepth_;
}

void AScoreOptions::setMaxPeakDepth(int maxPeakDepth)
{
   maxPeakDepth_ = maxPeakDepth;
}

double AScoreOptions::getTolerance() const
{
   return tolerance_;
}

void AScoreOptions::setTolerance(double tolerance)
{
   tolerance_ = tolerance;
}

const std::string& AScoreOptions::getUnitText() const
{
   return unitText_;
}

void AScoreOptions::setUnitText(const std::string& unitText)
{
   unitText_ = unitText;
}

Mass::Units AScoreOptions::getUnits() const
{
   return units_;
}

void AScoreOptions::setUnits(Mass::Units units)
{
   units_ = units;
}

int AScoreOptions::getWindow() const
{
   return window_;
}

void AScoreOptions::setWindow(int window)
{
   window_ = window;
}

bool AScoreOptions::getLowMassCutoff() const
{
   return lowMassCutoff_;
}

void AScoreOptions::setLowMassCutoff(bool lowMassCutoff)
{
   lowMassCutoff_ = lowMassCutoff;
}

double AScoreOptions::getFilterLowIntensity() const
{
   return filterLowIntensity_;
}

void AScoreOptions::setFilterLowIntensity(double filterLowIntensity)
{
   filterLowIntensity_ = filterLowIntensity;
}

Deisotoping AScoreOptions::getDeisotopingType() const
{
   return deisotopingType_;
}

void AScoreOptions::setDeisotopingType(Deisotoping deisotopingType)
{
   deisotopingType_ = deisotopingType;
}

bool AScoreOptions::getNoCterm() const
{
   return noCterm_;
}

void AScoreOptions::setNoCterm(bool noCterm)
{
   noCterm_ = noCterm;
}

bool AScoreOptions::getUseMobScore() const
{
   return useMobScore_;
}

void AScoreOptions::setUseMobScore(bool useMobScore)
{
   useMobScore_ = useMobScore;
}

bool AScoreOptions::getUseDeltaAscore() const
{
   return useDeltaAscore_;
}

void AScoreOptions::setUseDeltaAscore(bool useDeltaAscore)
{
   useDeltaAscore_ = useDeltaAscore;
}

char AScoreOptions::getSymbol() const
{
   return symbol_;
}

void AScoreOptions::setSymbol(char symbol)
{
   symbol_ = symbol;
}

const std::string& AScoreOptions::getResidues() const
{
   return residues_;
}

void AScoreOptions::setResidues(const std::string& residues)
{
   residues_ = residues;
}

const std::string& AScoreOptions::getOut() const
{
   return out_;
}

void AScoreOptions::setOut(const std::string& out)
{
   out_ = out;
}

int AScoreOptions::getMaxPeptides() const
{
   return maxPeptides_;
}

void AScoreOptions::setMaxPeptides(int maxPeptides)
{
   maxPeptides_ = maxPeptides;
}

int AScoreOptions::getMz() const
{
   return mz_;
}

void AScoreOptions::setMz(int mz)
{
   mz_ = mz;
}

const std::string& AScoreOptions::getPeptide() const
{
   return peptide_;
}

void AScoreOptions::setPeptide(const std::string& peptide)
{
   peptide_ = peptide;
}

int AScoreOptions::getScan() const
{
   return scan_;
}

void AScoreOptions::setScan(int scan)
{
   scan_ = scan;
}

const std::string& AScoreOptions::getPeptidesFile() const
{
   return peptidesFile_;
}

void AScoreOptions::setPeptidesFile(const std::string& peptidesFile)
{
   peptidesFile_ = peptidesFile;
}

AminoAcidMasses& AScoreOptions::getMasses()
{
   return masses_;
}

const AminoAcidMasses& AScoreOptions::getMasses() const
{
   return masses_;
}

void AScoreOptions::setMasses(const AminoAcidMasses& masses)
{
   masses_ = masses;
}

} // namespace AScoreProCpp