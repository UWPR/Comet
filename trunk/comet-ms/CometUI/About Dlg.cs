using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Windows.Forms;
using CometWrapper;

namespace CometUI
{
    public partial class AboutDlg : Form
    {
        public AboutDlg()
        {
            InitializeComponent();
            var searchManager = new CometSearchManagerWrapper();
            String cometVersion = String.Empty;
            if (searchManager.GetParamValue("# comet_version ", ref cometVersion))
                labelCometEngineVersion.Text = cometVersion;
            Version version = typeof (CometUI).Assembly.GetName().Version;
            labelCometUIVersion.Text = version.ToString();
        }

        private void AboutDlgLoad(object sender, EventArgs e)
        {
            linkLabel1.Links.Add(0, 41, "http://proteomicsresource.washington.edu/");
        }

        private void LinkLabel1LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            System.Diagnostics.Process.Start(e.Link.LinkData.ToString());
        }

        private void BtnOkClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
        }
    }
}
