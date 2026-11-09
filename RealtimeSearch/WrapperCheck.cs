namespace RealTimeSearch
{
   using System;
   using System.Collections.Generic;
   using System.Globalization;
   using System.IO;
   using System.Linq;

   using CometWrapper;

   /// <summary>
   /// "RealtimeSearch.exe wrappercheck &lt;database.idx&gt; &lt;fixture.txt&gt; &lt;expected_peptide&gt;":
   /// a self-check of CometWrapper's RTS entry points (test T78; branch code review item 11).
   /// Every RTS call is given null, empty, over-long and mismatched peak arrays and must return
   /// false with non-null, empty outputs, no exception, and a GetLastWrapperError() message;
   /// then valid calls -- including a reused buffer longer than iNumPeaks -- must find the
   /// expected peptide. The fixture is one rts_repro SPECTRUM block (scan, charge, m/z, peaks).
   /// Exit code 0 = all checks passed, 1 = a check failed, 2 = usage or setup error.
   /// </summary>
   internal static class WrapperCheck
   {
      private static int s_iChecks;
      private static int s_iFailures;

      private static void Check(bool bCondition, string sMessage)
      {
         ++s_iChecks;
         if (!bCondition)
            ++s_iFailures;
         Console.WriteLine((bCondition ? "  pass: " : "  FAIL: ") + sMessage);
      }

      private static string PlainPeptide(string sPeptide)
      {
         // "R.PEP[79.9663]TIDE.A" -> "PEPTIDE"
         string s = sPeptide;
         if (s.Length > 4 && s[1] == '.' && s[s.Length - 2] == '.')
            s = s.Substring(2, s.Length - 4);
         var sb = new System.Text.StringBuilder();
         int iDepth = 0;
         foreach (char c in s)
         {
            if (c == '[') ++iDepth;
            else if (c == ']') --iDepth;
            else if (iDepth == 0 && char.IsLetter(c)) sb.Append(c);
         }
         return sb.ToString();
      }

      private static bool ReadFixture(string sPath, out int iCharge, out double dMZ, out double[] pdMass, out double[] pdInten)
      {
         iCharge = 0;
         dMZ = 0.0;
         pdMass = null;
         pdInten = null;
         var inv = CultureInfo.InvariantCulture;
         string[] lines = File.ReadAllLines(sPath);
         for (int i = 0; i < lines.Length; ++i)
         {
            string[] f = lines[i].Split(new char[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
            if (f.Length >= 5 && f[0] == "SPECTRUM")
            {
               iCharge = int.Parse(f[2], inv);
               dMZ = double.Parse(f[3], inv);
               int n = int.Parse(f[4], inv);
               pdMass = new double[n];
               pdInten = new double[n];
               for (int k = 0; k < n; ++k)
               {
                  string[] p = lines[i + 1 + k].Split(new char[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries);
                  pdMass[k] = double.Parse(p[0], inv);
                  pdInten[k] = double.Parse(p[1], inv);
               }
               return true;
            }
         }
         return false;
      }

      // One RTS call with the given peak arrays; any exception escaping the wrapper is a failure.
      private delegate bool SearchCall(double[] pdMass, double[] pdInten, int iNumPeaks, out List<string> vPeptides, out int iOutputCount);

      private static void ExpectRejected(string sCall, string sCase, SearchCall call, double[] pdMass, double[] pdInten, int iNumPeaks,
         CometSearchManagerWrapper mgr, string sErrorFragment)
      {
         bool bOk = true;
         List<string> vPeptides = null;
         int iOutputCount = -1;
         string sException = null;
         try
         {
            bOk = call(pdMass, pdInten, iNumPeaks, out vPeptides, out iOutputCount);
         }
         catch (Exception ex)
         {
            sException = ex.GetType().Name + ": " + ex.Message;
         }
         string sError = mgr.GetLastWrapperError();
         Check(sException == null && !bOk && iOutputCount == 0 && sError != null && sError.Contains(sErrorFragment),
            string.Format("{0}, {1}: returns false with empty outputs and no exception (exception {2}, returned {3}, outputs {4}, error '{5}')",
               sCall, sCase, sException ?? "none", bOk, iOutputCount, sError ?? "null"));
      }

      private static void ExpectFound(string sCall, string sCase, SearchCall call, double[] pdMass, double[] pdInten, int iNumPeaks,
         CometSearchManagerWrapper mgr, string sExpected)
      {
         bool bOk = false;
         List<string> vPeptides = null;
         int iOutputCount = -1;
         string sException = null;
         try
         {
            bOk = call(pdMass, pdInten, iNumPeaks, out vPeptides, out iOutputCount);
         }
         catch (Exception ex)
         {
            sException = ex.GetType().Name + ": " + ex.Message;
         }
         string sFirst = (vPeptides != null && vPeptides.Count > 0) ? vPeptides[0] : "";
         Check(sException == null && bOk && PlainPeptide(sFirst) == sExpected && mgr.GetLastWrapperError() == null,
            string.Format("{0}, {1}: returns true and finds {2} (exception {3}, returned {4}, rank-1 '{5}', error '{6}')",
               sCall, sCase, sExpected, sException ?? "none", bOk, sFirst, mgr.GetLastWrapperError() ?? "null"));
      }

      public static int Run(string[] args)
      {
         if (args.Length < 4)
         {
            Console.WriteLine(" USAGE:  RealtimeSearch.exe wrappercheck <database.idx> <fixture.txt> <expected_peptide>");
            return 2;
         }
         string sDB = args[1];
         string sExpected = args[3];
         int iCharge;
         double dMZ;
         double[] pdMass, pdInten;
         if (!File.Exists(sDB) || !File.Exists(args[2]) || !ReadFixture(args[2], out iCharge, out dMZ, out pdMass, out pdInten))
         {
            Console.WriteLine(" Error: missing index or unreadable fixture");
            return 2;
         }

         using (var mgr = new CometSearchManagerWrapper())
         {
            var settings = new RealTimeSearch.SearchSettings();
            string sRef = sDB;
            double dLow = 0, dHigh = 0;
            int iThreads = 1;
            settings.ConfigureInputSettings(mgr, ref sRef, ref sDB, ref dLow, ref dHigh, ref iThreads, true, false, -1);
            mgr.SetParam("max_index_runtime", "0", 0);
            if (!mgr.InitializeSingleSpectrumSearch())
            {
               string sMsg = "";
               mgr.GetStatusMessage(ref sMsg);
               Console.WriteLine(" Error initializing search index: " + sMsg);
               return 2;
            }

            SearchCall multi = (double[] m, double[] i, int n, out List<string> vPep, out int iCount) =>
            {
               List<string> vProt;
               List<List<FragmentWrapper>> vFrag;
               List<ScoreWrapper> vScore;
               bool b = mgr.DoSingleSpectrumSearchMultiResults(1, iCharge, dMZ, m, i, n, out vPep, out vProt, out vFrag, out vScore);
               iCount = (vPep == null || vProt == null || vFrag == null || vScore == null) ? -1 : vPep.Count + vProt.Count + vFrag.Count + vScore.Count;
               return b;
            };
            SearchCall chimList = (double[] m, double[] i, int n, out List<string> vPep, out int iCount) =>
            {
               List<string> vProt;
               List<List<FragmentWrapper>> vFrag;
               List<ScoreWrapper> vScore;
               bool b = mgr.DoSingleSpectrumSearchChimeric(new double[] { dMZ }, new int[] { iCharge }, null, 0.0, 0.01, 5,
                  m, i, n, out vPep, out vProt, out vFrag, out vScore, out _);
               iCount = (vPep == null || vProt == null || vFrag == null || vScore == null) ? -1 : vPep.Count + vProt.Count + vFrag.Count + vScore.Count;
               return b;
            };
            SearchCall chimRange = (double[] m, double[] i, int n, out List<string> vPep, out int iCount) =>
            {
               List<string> vProt;
               List<List<FragmentWrapper>> vFrag;
               List<ScoreWrapper> vScore;
               bool b = mgr.DoSingleSpectrumSearchChimeric(dMZ, iCharge, iCharge, 0.0, 0.01, 5,
                  m, i, n, out vPep, out vProt, out vFrag, out vScore, out _);
               iCount = (vPep == null || vProt == null || vFrag == null || vScore == null) ? -1 : vPep.Count + vProt.Count + vFrag.Count + vScore.Count;
               return b;
            };
            SearchCall ms1 = (double[] m, double[] i, int n, out List<string> vPep, out int iCount) =>
            {
               List<ScoreWrapperMS1> vScore;
               bool b = mgr.DoMS1SearchMultiResults(10.0, 0.0, 1, 0.0, m, i, n, out vScore);
               vPep = null;
               iCount = (vScore == null) ? -1 : vScore.Count;
               return b;
            };

            int L = pdMass.Length;
            double[] pdShortInten = pdInten.Take(L - 5).ToArray();
            var calls = new List<KeyValuePair<string, SearchCall>>
            {
               new KeyValuePair<string, SearchCall>("DoSingleSpectrumSearchMultiResults", multi),
               new KeyValuePair<string, SearchCall>("DoSingleSpectrumSearchChimeric (list)", chimList),
               new KeyValuePair<string, SearchCall>("DoSingleSpectrumSearchChimeric (charge range)", chimRange),
               new KeyValuePair<string, SearchCall>("DoMS1SearchMultiResults", ms1),
            };
            foreach (var c in calls)
            {
               ExpectRejected(c.Key, "null mass array", c.Value, null, pdInten, L, mgr, "invalid peak arrays");
               ExpectRejected(c.Key, "null intensity array", c.Value, pdMass, null, L, mgr, "invalid peak arrays");
               ExpectRejected(c.Key, "empty arrays, iNumPeaks 0", c.Value, new double[0], new double[0], 0, mgr, "invalid peak arrays");
               ExpectRejected(c.Key, "empty arrays, iNumPeaks 5", c.Value, new double[0], new double[0], 5, mgr, "invalid peak arrays");
               ExpectRejected(c.Key, "iNumPeaks one more than the arrays", c.Value, pdMass, pdInten, L + 1, mgr, "invalid peak arrays");
               ExpectRejected(c.Key, "intensity array 5 shorter than iNumPeaks", c.Value, pdMass, pdShortInten, L, mgr, "invalid peak arrays");
            }

            // invalid candidate arrays (list form): 2 m/z values, 1 charge
            {
               List<string> vPep, vProt;
               List<List<FragmentWrapper>> vFrag;
               List<ScoreWrapper> vScore;
               bool b = true;
               string sEx = null;
               try
               {
                  b = mgr.DoSingleSpectrumSearchChimeric(new double[] { dMZ, dMZ + 1.0 }, new int[] { iCharge }, null, 0.0, 0.01, 5,
                     pdMass, pdInten, L, out vPep, out vProt, out vFrag, out vScore, out _);
               }
               catch (Exception ex)
               {
                  sEx = ex.Message;
                  vPep = null;
               }
               string sErr = mgr.GetLastWrapperError();
               Check(sEx == null && !b && vPep != null && vPep.Count == 0 && sErr != null && sErr.Contains("invalid candidate arrays"),
                  string.Format("DoSingleSpectrumSearchChimeric (list), 2 m/z and 1 charge: rejected (returned {0}, error '{1}')", b, sErr ?? "null"));
            }

            // valid calls; a reused buffer longer than iNumPeaks (20 trailing zeros) is accepted
            double[] pdMassPadded = pdMass.Concat(new double[20]).ToArray();
            double[] pdIntenPadded = pdInten.Concat(new double[20]).ToArray();
            for (int k = 0; k < 3; ++k)
            {
               ExpectFound(calls[k].Key, "valid arrays", calls[k].Value, pdMass, pdInten, L, mgr, sExpected);
               ExpectFound(calls[k].Key, "buffer 20 longer than iNumPeaks", calls[k].Value, pdMassPadded, pdIntenPadded, L, mgr, sExpected);
            }

            mgr.FinalizeSingleSpectrumSearch();
         }

         Console.WriteLine();
         Console.WriteLine(s_iFailures == 0 ? " WRAPPERCHECK PASSED: {0} checks" : " WRAPPERCHECK FAILED: {1} of {0} checks", s_iChecks, s_iFailures);
         return s_iFailures == 0 ? 0 : 1;
      }
   }
}
