// Copyright 2012-2026 Jimmy Eng
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Unit tests for MSReader's header-only mode (MSReader::setHeaderOnly() /
// lastReadHeaderOnly(), MSToolkit/src/MSToolkit/MSReader.cpp), the reader contract that
// CometPreprocess::ReadPrecursors() relies on for the fragindex_skipreadprecursors = 0
// precursor pre-read:
//
//   - a header-only mzXML read returns the scan's metadata (scan number, precursor m/z and
//     charge) without peaks and reports it through lastReadHeaderOnly();
//   - the same MSReader can switch to a full read of the same scan and back to header-only
//     reads without stale state in either direction;
//   - a read that did return peaks -- another file format, or header-only turned off --
//     never leaves lastReadHeaderOnly() set;
//   - many header-only reads in a row stay consistent (each returns the right scan).
//
// ReadPrecursors() drives exactly this sequence for spectra whose file gives no precursor
// charge (header-only read, full re-read of that scan, header-only read of the next scan);
// the Python test T61 covers it through a search, these tests assert the reader contract
// directly. The fixtures are written to the temp directory by the tests themselves.
//
// Framework: MiniTest.h (see TestCometSearchAndPreprocess.cpp, which also holds MINITEST_MAIN()).

#include "MiniTest.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "MSReader.h"
#include "Spectrum.h"

using namespace MSToolkit;

namespace
{
   const int kNumPeaksMs1 = 8;
   const int kNumPeaksScan2 = 16;
   const int kNumPeaksScan3 = 12;
   const int kNumPeaksMs2File = 3;

   std::string Base64(const std::string& strRaw)
   {
      static const char* szTbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      std::string strOut;
      size_t i = 0;
      for (; i + 2 < strRaw.size(); i += 3)
      {
         uint32_t v = ((uint8_t)strRaw[i] << 16) | ((uint8_t)strRaw[i + 1] << 8) | (uint8_t)strRaw[i + 2];
         strOut += szTbl[(v >> 18) & 63]; strOut += szTbl[(v >> 12) & 63]; strOut += szTbl[(v >> 6) & 63]; strOut += szTbl[v & 63];
      }
      if (i + 1 == strRaw.size())
      {
         uint32_t v = (uint8_t)strRaw[i] << 16;
         strOut += szTbl[(v >> 18) & 63]; strOut += szTbl[(v >> 12) & 63]; strOut += "==";
      }
      else if (i + 2 == strRaw.size())
      {
         uint32_t v = ((uint8_t)strRaw[i] << 16) | ((uint8_t)strRaw[i + 1] << 8);
         strOut += szTbl[(v >> 18) & 63]; strOut += szTbl[(v >> 12) & 63]; strOut += szTbl[(v >> 6) & 63]; strOut += '=';
      }
      return strOut;
   }

   // m/z, intensity pairs as 64-bit big-endian doubles ("network" byte order), base64 encoded
   std::string EncodePeaks(int iNumPeaks, double dFirstMz)
   {
      std::string strRaw;
      for (int i = 0; i < iNumPeaks; ++i)
      {
         const double d[2] = { dFirstMz + 37.5 * i, 100.0 + i };
         for (double v : d)
         {
            uint64_t u;
            std::memcpy(&u, &v, sizeof(u));
            for (int b = 7; b >= 0; --b)
               strRaw += (char)((u >> (8 * b)) & 0xFF);
         }
      }
      return Base64(strRaw);
   }

   // Indexed mzXML: scan 1 = MS1, scan 2 = MS2 with precursor charge 2, scan 3 = MS2 without a charge.
   void WriteMzxml(const std::string& strPath)
   {
      const std::string strHead =
         "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
         "<mzXML xmlns=\"http://sashimi.sourceforge.net/schema_revision/mzXML_3.2\">\n"
         "<msRun scanCount=\"3\" startTime=\"PT0S\" endTime=\"PT10S\">\n";

      struct ScanDef { int iNum; int iLevel; int iPeaks; double dPrecursorMz; const char* szCharge; };
      const ScanDef scans[] = {
         { 1, 1, kNumPeaksMs1, 0.0, "" },
         { 2, 2, kNumPeaksScan2, 465.748182, " precursorCharge=\"2\"" },
         { 3, 2, kNumPeaksScan3, 500.250000, "" } };

      std::string strBody;
      std::vector<size_t> vOffsets;
      for (const ScanDef& s : scans)
      {
         vOffsets.push_back(strHead.size() + strBody.size());
         strBody += "<scan num=\"" + std::to_string(s.iNum) + "\" msLevel=\"" + std::to_string(s.iLevel)
            + "\" peaksCount=\"" + std::to_string(s.iPeaks) + "\" polarity=\"+\" retentionTime=\"PT" + std::to_string(s.iNum)
            + ".0S\" lowMz=\"100\" highMz=\"2000\" basePeakMz=\"100\" basePeakIntensity=\"100\" totIonCurrent=\"1000\">\n";
         if (s.iLevel == 2)
         {
            char szMz[64];
            std::snprintf(szMz, sizeof(szMz), "%.6f", s.dPrecursorMz);
            strBody += std::string("<precursorMz precursorIntensity=\"1000\"") + s.szCharge + " activationMethod=\"CID\">" + szMz + "</precursorMz>\n";
         }
         strBody += "<peaks compressionType=\"none\" compressedLen=\"0\" precision=\"64\" byteOrder=\"network\" contentType=\"m/z-int\">"
            + EncodePeaks(s.iPeaks, 100.0 + 10 * s.iNum) + "</peaks>\n</scan>\n";
      }

      const std::string strEndRun = "</msRun>\n";
      const size_t iIndexPos = strHead.size() + strBody.size() + strEndRun.size();
      std::string strTail = strEndRun + "<index name=\"scan\">\n";
      for (size_t i = 0; i < vOffsets.size(); ++i)
         strTail += "<offset id=\"" + std::to_string(scans[i].iNum) + "\">" + std::to_string(vOffsets[i]) + "</offset>\n";
      strTail += "</index>\n<indexOffset>" + std::to_string(iIndexPos) + "</indexOffset>\n<sha1>" + std::string(40, '0') + "</sha1>\n</mzXML>\n";

      std::ofstream out(strPath, std::ios::binary);
      out << strHead << strBody << strTail;
   }

   void WriteMs2(const std::string& strPath)
   {
      std::ofstream out(strPath, std::ios::binary);
      out << "S\t1\t1\t500.000000\nZ\t2\t999.000000\n100.000000\t10.0\n200.000000\t20.0\n300.000000\t30.0\n";
   }

   // Writes the fixtures under the temp directory and removes them again when it goes out of scope.
   struct Fixtures
   {
      std::string strMzxml;
      std::string strMs2;

      explicit Fixtures(const char* szTag)
      {
         const std::filesystem::path dir = std::filesystem::temp_directory_path();
         strMzxml = (dir / (std::string("comet_msreader_") + szTag + ".mzXML")).string();
         strMs2 = (dir / (std::string("comet_msreader_") + szTag + ".ms2")).string();
         WriteMzxml(strMzxml);
         WriteMs2(strMs2);
      }

      ~Fixtures()
      {
         std::error_code ec;
         std::filesystem::remove(strMzxml, ec);
         std::filesystem::remove(strMs2, ec);
      }
   };
}

TEST(MSReaderHeaderOnlyTest, HeaderOnlyThenFullThenHeaderOnlyOnOneReader)
{
   Fixtures f("mixed");
   MSReader reader;
   Spectrum spec;
   reader.setFilter(MS2);

   // header-only: scan 2's metadata, no peaks
   reader.setHeaderOnly(true);
   EXPECT_TRUE(reader.readFile(f.strMzxml.c_str(), spec, 0));
   EXPECT_EQ(2, spec.getScanNumber());
   EXPECT_EQ(0, spec.size());
   EXPECT_TRUE(reader.lastReadHeaderOnly());
   EXPECT_EQ(1, spec.sizeZ());
   EXPECT_EQ(2, spec.atZ(0).z);
   EXPECT_TRUE(spec.getMZ() > 465.7 && spec.getMZ() < 465.8);

   // the same scan again with peaks, on the same reader
   reader.setHeaderOnly(false);
   EXPECT_TRUE(reader.readFile(f.strMzxml.c_str(), spec, 2));
   EXPECT_EQ(2, spec.getScanNumber());
   EXPECT_EQ(kNumPeaksScan2, spec.size());
   EXPECT_FALSE(reader.lastReadHeaderOnly());

   // and back to header-only for the next MS2 scan (no precursor charge in the file)
   reader.setHeaderOnly(true);
   EXPECT_TRUE(reader.readFile(NULL, spec));
   EXPECT_EQ(3, spec.getScanNumber());
   EXPECT_EQ(0, spec.size());
   EXPECT_TRUE(reader.lastReadHeaderOnly());
   EXPECT_EQ(0, spec.sizeZ());
   EXPECT_TRUE(spec.getMZ() > 500.2 && spec.getMZ() < 500.3);
}

TEST(MSReaderHeaderOnlyTest, FullReadsKeepPeaksAndNeverReportHeaderOnly)
{
   Fixtures f("full");
   MSReader reader;
   Spectrum spec;
   reader.setFilter(MS2);

   // default: header-only is off
   EXPECT_FALSE(reader.lastReadHeaderOnly());
   EXPECT_TRUE(reader.readFile(f.strMzxml.c_str(), spec, 0));
   EXPECT_EQ(2, spec.getScanNumber());
   EXPECT_EQ(kNumPeaksScan2, spec.size());
   EXPECT_FALSE(reader.lastReadHeaderOnly());

   EXPECT_TRUE(reader.readFile(NULL, spec));
   EXPECT_EQ(3, spec.getScanNumber());
   EXPECT_EQ(kNumPeaksScan3, spec.size());
   EXPECT_FALSE(reader.lastReadHeaderOnly());
}

TEST(MSReaderHeaderOnlyTest, FlagDoesNotLeakIntoOtherFormats)
{
   Fixtures f("formats");
   MSReader reader;
   Spectrum spec;
   reader.setFilter(MS2);
   reader.setHeaderOnly(true);

   // a header-only mzXML read sets the flag ...
   EXPECT_TRUE(reader.readFile(f.strMzxml.c_str(), spec, 0));
   EXPECT_EQ(0, spec.size());
   EXPECT_TRUE(reader.lastReadHeaderOnly());

   // ... a following ms2 read on the same reader returns its peaks (header-only is an mzML/mzXML
   // feature) and must clear it
   EXPECT_TRUE(reader.readFile(f.strMs2.c_str(), spec));
   EXPECT_EQ(kNumPeaksMs2File, spec.size());
   EXPECT_FALSE(reader.lastReadHeaderOnly());
}

TEST(MSReaderHeaderOnlyTest, Ms2FilesIgnoreHeaderOnly)
{
   Fixtures f("ms2");
   MSReader reader;
   Spectrum spec;
   reader.setHeaderOnly(true);

   EXPECT_TRUE(reader.readFile(f.strMs2.c_str(), spec));
   EXPECT_EQ(kNumPeaksMs2File, spec.size());
   EXPECT_FALSE(reader.lastReadHeaderOnly());
}

TEST(MSReaderHeaderOnlyTest, RepeatedHeaderOnlyReadsStayConsistent)
{
   Fixtures f("repeat");
   MSReader reader;
   Spectrum spec;
   reader.setFilter(MS2);
   reader.setHeaderOnly(true);

   // the same two scans over and over, by random access, the way the pre-read re-reads single scans
   EXPECT_TRUE(reader.readFile(f.strMzxml.c_str(), spec, 0));
   int iBad = 0;
   for (int i = 0; i < 200; ++i)
   {
      const int iScan = (i % 2 == 0) ? 3 : 2;
      if (!reader.readFile(f.strMzxml.c_str(), spec, iScan)
            || spec.getScanNumber() != iScan
            || spec.size() != 0
            || !reader.lastReadHeaderOnly()
            || spec.sizeZ() != (iScan == 2 ? 1 : 0))
      {
         ++iBad;
      }
   }
   EXPECT_EQ(0, iBad);

   // after all of that a full read still returns the whole spectrum
   reader.setHeaderOnly(false);
   EXPECT_TRUE(reader.readFile(f.strMzxml.c_str(), spec, 3));
   EXPECT_EQ(kNumPeaksScan3, spec.size());
   EXPECT_FALSE(reader.lastReadHeaderOnly());
}
