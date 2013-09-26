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
