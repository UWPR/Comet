using System;
using System.Collections.Generic;
using System.Windows.Forms;
using CometWrapper;

namespace CometUI
{
    public partial class CometMainForm : Form
    {
        private readonly CometSearchManagerWrapper _searchMgr;
        public CometMainForm()
        {
            InitializeComponent();

            _searchMgr = new CometSearchManagerWrapper();
        }

        private void BtnTestClick(object sender, EventArgs e)
        {
            if (!_searchMgr.DoSearch())
            {
                String strErrorMsg = String.Empty;
                if (_searchMgr.GetErrorMessage(ref strErrorMsg))
                {
                    MessageBox.Show(strErrorMsg);
                }
            }

            _searchMgr.SetOutputFileBaseName("MyTestBaseName");

            List<InputFileInfoWrapper> inputFiles = new List<InputFileInfoWrapper>();
            InputFileInfoWrapper inputFile = new InputFileInfoWrapper();
            inputFile.set_InputType(InputType.MZXML);
            inputFile.set_FirstScan(0);
            inputFile.set_LastScan(0);
            inputFile.set_AnalysisType(AnalysisType.EntireFile);
            inputFile.set_FileName("test.MZXML");
            inputFile.set_BaseName("test");
            inputFiles.Add(inputFile);
            _searchMgr.AddInputFiles(inputFiles);

            _searchMgr.SetParam("num_threads", "1", 1);
            int numThreads = 0;
            _searchMgr.GetParamValue("num_threads", ref numThreads);

            IntRangeWrapper scanRange = new IntRangeWrapper(0, 0);
            _searchMgr.SetParam("scan_range", "0 0", scanRange);
            IntRangeWrapper scanRangeGet = new IntRangeWrapper(5, 544);
            _searchMgr.GetParamValue("scan_range", ref scanRangeGet);

            DoubleRangeWrapper digestMassRange = new DoubleRangeWrapper(0.0, 678.9);
            _searchMgr.SetParam("digest_mass_range", "600.0, 5000.0", digestMassRange);
            DoubleRangeWrapper digestMassRangeGet = new DoubleRangeWrapper(0.0, 0.0);
            _searchMgr.GetParamValue("digest_mass_range", ref digestMassRangeGet);

            VarModsWrapper varMods = new VarModsWrapper();
            varMods.set_BinaryMod(1);
            varMods.set_MaxNumVarModAAPerMod(5);
            varMods.set_VarModMass(15.9949);
            varMods.set_VarModChar("M");
            _searchMgr.SetParam("variable_mod1", "1, 5, 15.9949, M", varMods);
            VarModsWrapper varModsGet = new VarModsWrapper();
            _searchMgr.GetParamValue("variable_mod1", ref varModsGet);

            EnzymeInfoWrapper enzymeInfo = new EnzymeInfoWrapper();
            enzymeInfo.set_AllowedMissedCleavge(3);
            enzymeInfo.set_SearchEnzymeOffSet(1);
            enzymeInfo.set_SearchEnzymeName("Trypsin");
            enzymeInfo.set_SearchEnzymeBreakAA("KR");
            enzymeInfo.set_SearchEnzymeNoBreakAA("P");
            _searchMgr.SetParam("[COMET_ENZYME_INFO]", "1.  Trypsin                1      KR          P", enzymeInfo);
            EnzymeInfoWrapper ezymeInfoGet = new EnzymeInfoWrapper();
            _searchMgr.GetParamValue("[COMET_ENZYME_INFO]", ref ezymeInfoGet);

            _searchMgr.SetParam("peptide_mass_tolerance", "3.00", 3.00);
            double dPepMassTol = 0;
            _searchMgr.GetParamValue("peptide_mass_tolerance", ref dPepMassTol);

            if (_searchMgr.SetParam("database_name", "18mix.fasta", "18mix.fasta"))
            {
                String value = String.Empty;
                _searchMgr.GetParamValue("database_name", ref value);
                _searchMgr.DoSearch();
            }
        }
    }
}
