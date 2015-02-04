using System;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.ViewResults
{
    public partial class ViewResultsSummaryOptionsControl : UserControl
    {
        public String ErrorMessage { get; private set; }

        private static readonly string[] QuantitationTools = new[] {"xpress","asapratio","libra"};

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

        public bool UpdateSummaryOptions(String resultsPepXMLFile)
        {
            ErrorMessage = String.Empty;
            UpdatePepXMLFileCombo(resultsPepXMLFile);

            if (!UpdateSearchSummaryLabel(resultsPepXMLFile))
            {
                return false;
            }

            return true;
        }

        private void UpdatePepXMLFileCombo(String resultsPepXMLFile)
        {
            pepXMLFileCombo.Text = resultsPepXMLFile;
        }

        private bool UpdateSearchSummaryLabel(String resultsPepXMLFile)
        {
            String resultsFile = resultsPepXMLFile;
            if (String.Empty == resultsFile)
            {
                // If there is no results file, clear the search summary display
                searchResultsSummaryLabel.Text = String.Empty;
                return true;
            }

            var searchSummary = String.Empty;
            PepXMLReader pepXMLReader;
            // Create a reader for the results file
            try
            {
                pepXMLReader = new PepXMLReader(resultsFile);
            }
            catch (Exception e)
            {
                ErrorMessage =
                    Resources.
                        ViewResultsSummaryOptionsControl_UpdateSearchSummaryLabel_Could_not_read_the_results_pep_xml_file__ +
                    e.Message;
                return false;
            }

            searchSummary += pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary/sample_enzyme", "name");
            searchSummary += " digest, ";

            searchSummary += pepXMLReader.ReadAttributeFromFirstMatchingNode("/msms_pipeline_analysis/msms_run_summary/search_summary", "search_engine");
            searchSummary += " search engine, ";

            // Read the quantitation tool name, if there is one.
            var quantitationTool = GetQuantitationTool(pepXMLReader);
            if (quantitationTool.Equals(String.Empty))
            {
                quantitationTool = "[none]";
            }
            searchSummary += "quantitation: " + quantitationTool;
            
            // Display the search summary
            searchResultsSummaryLabel.Text = searchSummary;

            return true;
        }

        private String GetQuantitationTool(PepXMLReader pepXMLReader)
        {
            String quantitationTool = String.Empty;
            var analysisSummaryNodeIterator = pepXMLReader.ReadNodes("/msms_pipeline_analysis/analysis_summary");
            while (analysisSummaryNodeIterator.MoveNext())
            {
                var analysis = pepXMLReader.ReadAttribute(analysisSummaryNodeIterator.Current, "analysis");
                if ((String.Empty != analysis) && IsQuantitationTool(analysis.ToLower()))
                {
                    quantitationTool = analysis;
                }
            }

            return quantitationTool;
        }

        private bool IsQuantitationTool(String toolName)
        {
            var caseInsensitiveToolName = toolName.ToLower();
            return QuantitationTools.Any(name => name == caseInsensitiveToolName);
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
