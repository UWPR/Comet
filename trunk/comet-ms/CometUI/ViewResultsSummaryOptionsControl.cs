using System;
using System.IO;
using System.Windows.Forms;

namespace CometUI
{
    public partial class ViewResultsSummaryOptionsControl : UserControl
    {
        private ViewSearchResultsControl ViewSearchResultsControl { get; set; }

        public ViewResultsSummaryOptionsControl(ViewSearchResultsControl parent)
        {
            InitializeComponent();

            ViewSearchResultsControl = parent;
        }

        private void BtnBrowsePepXMLFileClick(object sender, EventArgs e)
        {
            var pepXMLFile = CometUI.ShowOpenPepXMLFile();
            if (null != pepXMLFile)
            {
                pepXMLFileCombo.Text = pepXMLFile;
            }
        }

        public void UpdateSummaryOptions()
        {
            pepXMLFileCombo.Text = ViewSearchResultsControl.ResultsPepXMLFile;

            searchSummaryLabel.Text = "trypsin digest, SEQUEST search engine, quantitation: xpress";
        }

        private void BtnUpdateResultsClick(object sender, EventArgs e)
        {
            if (String.Empty != pepXMLFileCombo.Text && !File.Exists(pepXMLFileCombo.Text))
            {
                pepXMLFileCombo.Text = String.Empty;
            }
            ViewSearchResultsControl.UpdateViewSearchResults(pepXMLFileCombo.Text);
        }
    }
}
