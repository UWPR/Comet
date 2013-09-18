using System;
using System.Windows.Forms;
using CometWrapper;

namespace CometUI
{
    public partial class MainForm : Form
    {
        private CometSearchManagerWrapper _searchMgr;
        public MainForm()
        {
            InitializeComponent();

            _searchMgr = new CometSearchManagerWrapper();
        }

        private void BtnTestClick(object sender, EventArgs e)
        {
            if (_searchMgr.SetParam("database_name", "18mix.fasta", "18mix.fasta"))
            {
                _searchMgr.DoSearch();
            }
        }
    }
}
