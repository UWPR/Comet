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
            _searchMgr.DoSearch();
        }
    }
}
