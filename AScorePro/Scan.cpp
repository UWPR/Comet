#include "Scan.h"
#include "Centroid.h"
#include "Precursor.h"
#include <algorithm> // For any STL algorithms if needed

namespace AScoreProCpp
{

Scan::Scan() :
   scanNumber_(0),
   scanEvent_(0),
   msOrder_(0),
   peakCount_(0),
   masterIndex_(0),
   ionInjectionTime_(0.0),
   elapsedScanTime_(0.0),
   retentionTime_(0.0),
   startMz_(0.0),
   endMz_(0.0),
   lowestMz_(0.0),
   highestMz_(0.0),
   basePeakMz_(0.0),
   basePeakIntensity_(0.0),
   faimsCV_(0),
   totalIonCurrent_(0.0),
   collisionEnergy_(0.0),
   precursorMasterScanNumber_(0)
{
}

Scan::~Scan()
{
   // Clear vectors
   centroids_.clear();
   precursors_.clear();
}

Scan::Scan(const Scan& other) :
   scanNumber_(other.scanNumber_),
   scanEvent_(other.scanEvent_),
   msOrder_(other.msOrder_),
   peakCount_(other.peakCount_),
   masterIndex_(other.masterIndex_),
   ionInjectionTime_(other.ionInjectionTime_),
   elapsedScanTime_(other.elapsedScanTime_),
   scanType_(other.scanType_),
   detectorType_(other.detectorType_),
   filterLine_(other.filterLine_),
   description_(other.description_),
   retentionTime_(other.retentionTime_),
   startMz_(other.startMz_),
   endMz_(other.endMz_),
   lowestMz_(other.lowestMz_),
   highestMz_(other.highestMz_),
   basePeakMz_(other.basePeakMz_),
   basePeakIntensity_(other.basePeakIntensity_),
   faimsCV_(other.faimsCV_),
   totalIonCurrent_(other.totalIonCurrent_),
   collisionEnergy_(other.collisionEnergy_),
   precursorMasterScanNumber_(other.precursorMasterScanNumber_),
   precursorActivationMethod_(other.precursorActivationMethod_),
   centroids_(other.centroids_),
   precursors_(other.precursors_)
{
}

Scan& Scan::operator=(const Scan& other)
{
   if (this != &other)
   {
      scanNumber_ = other.scanNumber_;
      scanEvent_ = other.scanEvent_;
      msOrder_ = other.msOrder_;
      peakCount_ = other.peakCount_;
      masterIndex_ = other.masterIndex_;
      ionInjectionTime_ = other.ionInjectionTime_;
      elapsedScanTime_ = other.elapsedScanTime_;
      scanType_ = other.scanType_;
      detectorType_ = other.detectorType_;
      filterLine_ = other.filterLine_;
      description_ = other.description_;
      retentionTime_ = other.retentionTime_;
      startMz_ = other.startMz_;
      endMz_ = other.endMz_;
      lowestMz_ = other.lowestMz_;
      highestMz_ = other.highestMz_;
      basePeakMz_ = other.basePeakMz_;
      basePeakIntensity_ = other.basePeakIntensity_;
      faimsCV_ = other.faimsCV_;
      totalIonCurrent_ = other.totalIonCurrent_;
      collisionEnergy_ = other.collisionEnergy_;
      precursorMasterScanNumber_ = other.precursorMasterScanNumber_;
      precursorActivationMethod_ = other.precursorActivationMethod_;
      centroids_ = other.centroids_;
      precursors_ = other.precursors_;
   }
   return *this;
}

Scan Scan::clone() const
{
   return Scan(*this);
}

// Getter and setter implementations
int Scan::getScanNumber() const
{
   return scanNumber_;
}

void Scan::setScanNumber(int scanNumber)
{
   scanNumber_ = scanNumber;
}

int Scan::getScanEvent() const
{
   return scanEvent_;
}

void Scan::setScanEvent(int scanEvent)
{
   scanEvent_ = scanEvent;
}

int Scan::getMsOrder() const
{
   return msOrder_;
}

void Scan::setMsOrder(int msOrder)
{
   msOrder_ = msOrder;
}

int Scan::getPeakCount() const
{
   return peakCount_;
}

void Scan::setPeakCount(int peakCount)
{
   peakCount_ = peakCount;
}

int Scan::getMasterIndex() const
{
   return masterIndex_;
}

void Scan::setMasterIndex(int masterIndex)
{
   masterIndex_ = masterIndex;
}

double Scan::getIonInjectionTime() const
{
   return ionInjectionTime_;
}

void Scan::setIonInjectionTime(double ionInjectionTime)
{
   ionInjectionTime_ = ionInjectionTime;
}

double Scan::getElapsedScanTime() const
{
   return elapsedScanTime_;
}

void Scan::setElapsedScanTime(double elapsedScanTime)
{
   elapsedScanTime_ = elapsedScanTime;
}

const std::string& Scan::getScanType() const
{
   return scanType_;
}

void Scan::setScanType(const std::string& scanType)
{
   scanType_ = scanType;
}

const std::string& Scan::getDetectorType() const
{
   return detectorType_;
}

void Scan::setDetectorType(const std::string& detectorType)
{
   detectorType_ = detectorType;
}

const std::string& Scan::getFilterLine() const
{
   return filterLine_;
}

void Scan::setFilterLine(const std::string& filterLine)
{
   filterLine_ = filterLine;
}

const std::string& Scan::getDescription() const
{
   return description_;
}

void Scan::setDescription(const std::string& description)
{
   description_ = description;
}

double Scan::getRetentionTime() const
{
   return retentionTime_;
}

void Scan::setRetentionTime(double retentionTime)
{
   retentionTime_ = retentionTime;
}

double Scan::getStartMz() const
{
   return startMz_;
}

void Scan::setStartMz(double startMz)
{
   startMz_ = startMz;
}

double Scan::getEndMz() const
{
   return endMz_;
}

void Scan::setEndMz(double endMz)
{
   endMz_ = endMz;
}

double Scan::getLowestMz() const
{
   return lowestMz_;
}

void Scan::setLowestMz(double lowestMz)
{
   lowestMz_ = lowestMz;
}

double Scan::getHighestMz() const
{
   return highestMz_;
}

void Scan::setHighestMz(double highestMz)
{
   highestMz_ = highestMz;
}

double Scan::getBasePeakMz() const
{
   return basePeakMz_;
}

void Scan::setBasePeakMz(double basePeakMz)
{
   basePeakMz_ = basePeakMz;
}

double Scan::getBasePeakIntensity() const
{
   return basePeakIntensity_;
}

void Scan::setBasePeakIntensity(double basePeakIntensity)
{
   basePeakIntensity_ = basePeakIntensity;
}

int Scan::getFaimsCV() const
{
   return faimsCV_;
}

void Scan::setFaimsCV(int faimsCV)
{
   faimsCV_ = faimsCV;
}

double Scan::getTotalIonCurrent() const
{
   return totalIonCurrent_;
}

void Scan::setTotalIonCurrent(double totalIonCurrent)
{
   totalIonCurrent_ = totalIonCurrent;
}

double Scan::getCollisionEnergy() const
{
   return collisionEnergy_;
}

void Scan::setCollisionEnergy(double collisionEnergy)
{
   collisionEnergy_ = collisionEnergy;
}

int Scan::getPrecursorMasterScanNumber() const
{
   return precursorMasterScanNumber_;
}

void Scan::setPrecursorMasterScanNumber(int precursorMasterScanNumber)
{
   precursorMasterScanNumber_ = precursorMasterScanNumber;
}

const std::string& Scan::getPrecursorActivationMethod() const
{
   return precursorActivationMethod_;
}

void Scan::setPrecursorActivationMethod(const std::string& precursorActivationMethod)
{
   precursorActivationMethod_ = precursorActivationMethod;
}

std::vector<Centroid>& Scan::getCentroids()
{
   return centroids_;
}

const std::vector<Centroid>& Scan::getCentroids() const
{
   return centroids_;
}

void Scan::setCentroids(const std::vector<Centroid>& centroids)
{
   centroids_ = centroids;
}

std::vector<Precursor>& Scan::getPrecursors()
{
   return precursors_;
}

const std::vector<Precursor>& Scan::getPrecursors() const
{
   return precursors_;
}

void Scan::setPrecursors(const std::vector<Precursor>& precursors)
{
   precursors_ = precursors;
}

} // namespace AScoreProCpp