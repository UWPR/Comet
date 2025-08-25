#pragma once

#ifndef _SCAN_H_
#define _SCAN_H_

#include <string>
#include <vector>
#include <memory>
#include "API.h"
#include "Centroid.h"
#include "Precursor.h"

namespace AScoreProCpp
{

/**
 * Class to harvest scan information
 */
class ASCORE_API Scan
{
public:
   Scan();
   virtual ~Scan();

   // Copy constructor and assignment operator
   Scan(const Scan& other);
   Scan& operator=(const Scan& other);

   // Create a copy of this scan
   Scan clone() const;

   // Getters and setters
   int getScanNumber() const;
   void setScanNumber(int scanNumber);

   int getScanEvent() const;
   void setScanEvent(int scanEvent);

   int getMsOrder() const;
   void setMsOrder(int msOrder);

   int getPeakCount() const;
   void setPeakCount(int peakCount);

   int getMasterIndex() const;
   void setMasterIndex(int masterIndex);

   double getIonInjectionTime() const;
   void setIonInjectionTime(double ionInjectionTime);

   double getElapsedScanTime() const;
   void setElapsedScanTime(double elapsedScanTime);

   const std::string& getScanType() const;
   void setScanType(const std::string& scanType);

   const std::string& getDetectorType() const;
   void setDetectorType(const std::string& detectorType);

   const std::string& getFilterLine() const;
   void setFilterLine(const std::string& filterLine);

   const std::string& getDescription() const;
   void setDescription(const std::string& description);

   double getRetentionTime() const;
   void setRetentionTime(double retentionTime);

   double getStartMz() const;
   void setStartMz(double startMz);

   double getEndMz() const;
   void setEndMz(double endMz);

   double getLowestMz() const;
   void setLowestMz(double lowestMz);

   double getHighestMz() const;
   void setHighestMz(double highestMz);

   double getBasePeakMz() const;
   void setBasePeakMz(double basePeakMz);

   double getBasePeakIntensity() const;
   void setBasePeakIntensity(double basePeakIntensity);

   int getFaimsCV() const;
   void setFaimsCV(int faimsCV);

   double getTotalIonCurrent() const;
   void setTotalIonCurrent(double totalIonCurrent);

   double getCollisionEnergy() const;
   void setCollisionEnergy(double collisionEnergy);

   int getPrecursorMasterScanNumber() const;
   void setPrecursorMasterScanNumber(int precursorMasterScanNumber);

   const std::string& getPrecursorActivationMethod() const;
   void setPrecursorActivationMethod(const std::string& precursorActivationMethod);

   std::vector<Centroid>& getCentroids();
   const std::vector<Centroid>& getCentroids() const;
   void setCentroids(const std::vector<Centroid>& centroids);

   // Make sure we have complete type information for std::vector<Precursor>
   std::vector<Precursor>& getPrecursors();
   const std::vector<Precursor>& getPrecursors() const;
   void setPrecursors(const std::vector<Precursor>& precursors);

private:
   // Current scan number
   int scanNumber_;

   // Scan event order
   int scanEvent_;

   // The scan order (e.g. 1 = MS1, 2 = MS2, 3 = MS3, MSn)
   int msOrder_;

   // Total number of peaks in the current scan
   int peakCount_;

   // Thermo variable for master scan number
   int masterIndex_;

   // Injection time used to acquire the scan ions (milliseconds, max = 5000)
   double ionInjectionTime_;

   // Total time, including injection time, to acquire the current scan (milliseconds)
   double elapsedScanTime_;

   // String description of scan type
   std::string scanType_;

   // Detector or mass analyzer used for the scan (e.g. ITMS or FTMS)
   std::string detectorType_;

   // Scan filter line from RAW file
   std::string filterLine_;

   // "Scan Description" field from the scan header
   std::string description_;

   // Scan retention time (minutes)
   double retentionTime_;

   // Mz that scan starts at
   double startMz_;

   // Mz that scan ends at
   double endMz_;

   // Lowest Mz observed in scan
   double lowestMz_;

   // Highest Mz observed in scan
   double highestMz_;

   // The most intense Mz peak in the scan
   double basePeakMz_;

   // Intensity of the most intense peak
   double basePeakIntensity_;

   // FAIMS compensation voltage, if used (in volts)
   int faimsCV_;

   // Total ion current for the current scan
   double totalIonCurrent_;

   // If a dependent scan, the fragmentation energy used
   double collisionEnergy_;

   // If a dependent scan, the parent scan number
   int precursorMasterScanNumber_;

   // If a dependent scan, the activation method used to generate the scan fragments
   std::string precursorActivationMethod_;

   // The observed centroid peaks in the scan
   std::vector<Centroid> centroids_;

   // Holds precursor information for dependent scans
   std::vector<Precursor> precursors_;
};

} // namespace AScoreProCpp

#endif // _SCAN_H_
