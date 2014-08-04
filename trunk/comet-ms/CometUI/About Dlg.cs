using System;
using System.Diagnostics;
using System.Windows.Forms;
using CometUI.Properties;
using CometWrapper;

namespace CometUI
{
    public partial class AboutDlg : Form
    {
        public AboutDlg()
        {
            InitializeComponent();

            SetAboutText();

            SetCometVersion();

            SetLinks();
        }

        private void SetAboutText()
        {
            textBoxAboutComet.Text =
                Resources.AboutDlg_SetAboutText_Comet_UI_provides_a_graphical_user_interface_to_run_Comet_searches__set_and_save_search_parameters__and_import_and_export_the_parameters_;

        }

        private void SetCometVersion()
        {
            var searchManager = new CometSearchManagerWrapper();
            String cometVersion = String.Empty;
            if (searchManager.GetParamValue("# comet_version ", ref cometVersion))
            {
                labelCometEngineVersion.Text = cometVersion;
            }

            Version version = typeof(CometUI).Assembly.GetName().Version;
            labelCometUIVersion.Text = version.ToString();
        }

        private void SetLinks()
        {
            const string uwprLink = "http://proteomicsresource.washington.edu/";
            linkLabelUWPR.Links.Add(0, uwprLink.Length, uwprLink);
        }

        private void BtnOkClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
        }

        private void LinkLabelUWPRLinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start(e.Link.LinkData.ToString());
        }
    }
}
