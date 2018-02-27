using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CometWrapper;

namespace RealtimeSearch
{
   class Search
   {
      static void Main(string[] args)
      {
         String sTmp;
         int iTmp;
         double dTmp;

         CometSearchManagerWrapper SearchMgr;
         SearchMgr = new CometSearchManagerWrapper();

         // can set other parameters this way
         string sDB = "C:\\Users\\Jimmy\\Desktop\\work\\cometsearch\\human.fasta.idx";
         SearchMgr.SetParam("database_name", sDB, sDB);

         dTmp = 20.0;
         sTmp = "20.0";
         SearchMgr.SetParam("peptide_mass_tolerance", sDB, dTmp);

         iTmp = 2; // ppm
         sTmp = "2";
         SearchMgr.SetParam("peptide_mass_units", sTmp, iTmp);

         iTmp = 1; // m/z tolerance
         sTmp = "1";
         SearchMgr.SetParam("precursor_tolerance_type", sTmp, iTmp);

         iTmp = 3; // isotope error 0 to 3
         sTmp = "3";
         SearchMgr.SetParam("isotope_error", sTmp, iTmp);


//         SearchMgr.DoSearch();

         int iPrecursorCharge = 2;
         double dMZ = 564.785827;
         SearchMgr.DoSingleSpectrumSearch(iPrecursorCharge, dMZ);
      }
   }
}
