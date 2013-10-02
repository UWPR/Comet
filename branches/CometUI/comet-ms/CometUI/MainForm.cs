using System;
using System.Windows.Forms;
using CometWrapper;

namespace CometUI
{
    public partial class MainForm : Form
    {
        private readonly CometSearchManagerWrapper _searchMgr;
        public MainForm()
        {
            InitializeComponent();

            _searchMgr = new CometSearchManagerWrapper();
        }

        private void BtnTestClick(object sender, EventArgs e)
        {
            _searchMgr.SetOutputFileBaseName("MyTestBaseName");

            int numThreads = 0;
            _searchMgr.SetParam("num_threads", "5", 5);
            _searchMgr.GetParamValue("num_threads", ref numThreads);

            IntRangeWrapper scanRange = new IntRangeWrapper(0, 544);
            _searchMgr.SetParam("scan_range", "0 544", scanRange);
            scanRange.set_iStart(20);
            scanRange.set_iEnd(4000);
            _searchMgr.GetParamValue("scan_range", ref scanRange);


            DoubleRangeWrapper digestMassRange = new DoubleRangeWrapper(0.0, 678.9);
            _searchMgr.SetParam("digest_mass_range", "0.0, 678.9", digestMassRange);
            digestMassRange.set_dStart(999.99);
            digestMassRange.set_dEnd(9999.99);
            _searchMgr.GetParamValue("digest_mass_range", ref digestMassRange);

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
            //enzymeInfo.set_SampleEnzymeOffSet(3);
            //enzymeInfo.set_SampleEnzymeName("Lys_C");
            //enzymeInfo.set_SampleEnzymeBreakAA("K");
            //enzymeInfo.set_SampleEnzymeNoBreakAA("P");
            _searchMgr.SetParam("[COMET_ENZYME_INFO]", "1.  Trypsin                1      KR          P", enzymeInfo);
            EnzymeInfoWrapper ezymeInfoGet = new EnzymeInfoWrapper();
            _searchMgr.GetParamValue("[COMET_ENZYME_INFO]", ref ezymeInfoGet);

            double dPepMassTol = 0;
            _searchMgr.SetParam("peptide_mass_tolerance", "2", (double)2);
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
