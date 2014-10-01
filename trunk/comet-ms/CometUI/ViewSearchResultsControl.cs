using System;
using System.Drawing;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI
{
    public partial class ViewSearchResultsControl : UserControl
    {
        public String ResultsPepXMLFile { get; set; }
        
        private CometUI CometUI { get; set; }
        private bool OptionsPanelShown { get; set; }
        private ViewResultsSummaryOptionsControl ViewResultsSummaryOptionsControl { get; set; }

        public ViewSearchResultsControl(CometUI parent)
        {
            InitializeComponent();

            CometUI = parent;

            ViewResultsSummaryOptionsControl = new ViewResultsSummaryOptionsControl(this)
                                                   {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };

            summaryTabPage.Controls.Add(ViewResultsSummaryOptionsControl);

            ShowViewOptionsPanel();

            UpdateViewSearchResults(String.Empty);
        }

        private void ShowViewOptionsPanel()
        {
            showHideOptionsLabel.Text = Resources.ViewSearchResultsControl_ShowViewOptionsPanel_Hide_options;
            showOptionsPanel.Visible = true;
            hideOptionsGroupBox.Visible = false;
            OptionsPanelShown = true;
            resultsListPanel.Location = resultsListPanelNormal.Location;
            resultsListPanel.Size = resultsListPanelNormal.Size;
        }

        private void HideViewOptionsPanel()
        {
            showHideOptionsLabel.Text = Resources.ViewSearchResultsControl_HideViewOptionsPanel_Show_options;
            showOptionsPanel.Visible = false;
            hideOptionsGroupBox.Visible = true;
            OptionsPanelShown = false;
            resultsListPanel.Location = resultsListPanelFull.Location;
            resultsListPanel.Size = resultsListPanelFull.Size;
            linkLabelPage9.Hide();
            pageNumbersPanel.Width -= linkLabelPage9.Width;
            linkLabelPage10.Hide();
            pageNumbersPanel.Width -= linkLabelPage10.Width;
            pageNavPanel.Refresh();

        }

        private void ShowHideOptionsBtnClick(object sender, EventArgs e)
        {
            if (OptionsPanelShown)
            {
                HideViewOptionsPanel();
            }
            else
            {
                ShowViewOptionsPanel();
            }
        }

        private void ShowResultsListPanel(bool show)
        {
            if (show)
            {
                resultsListPanel.Show();
            }
            else
            {
                resultsListPanel.Hide();
            }
        }

        public void UpdateViewSearchResults(String resultsPepXMLFile)
        {
            if (null != resultsPepXMLFile)
            {
                ResultsPepXMLFile = resultsPepXMLFile;
                ShowResultsListPanel(String.Empty != ResultsPepXMLFile);
                ViewResultsSummaryOptionsControl.UpdateSummaryOptions();
            }
        }
    }
}
