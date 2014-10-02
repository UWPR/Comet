using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using System.Xml.Linq;

namespace CometUI
{
    public partial class ViewResultsSummaryOptionsControl : UserControl
    {
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

        public void UpdateSummaryOptions()
        {
            UpdatePepXMLFileCombo();
            UpdateSearchSummaryLabel();
        }

        private void UpdatePepXMLFileCombo()
        {
            pepXMLFileCombo.Text = ViewSearchResultsControl.ResultsPepXMLFile;
        }

        private void UpdateSearchSummaryLabel()
        {
            // If there is no results file, the search summary display is blank
            String searchSummary = String.Empty;

            String resultsFile = ViewSearchResultsControl.ResultsPepXMLFile;
            if (String.Empty != resultsFile)
            {
                // Create a reader for the results file
                var pepXMLReader = new PepXMLReader(resultsFile);
                
                //// Read the digest enzyme name
                //IEnumerable<XElement> sampleEzymeElements = pepXMLReader.ReadElements("sample_enzyme");
                //XAttribute firstEnzymeName = pepXMLReader.ReadFirstAttribute(sampleEzymeElements, "name");
                //if (null != firstEnzymeName)
                //{
                //    searchSummary += (String)firstEnzymeName;
                //}

                // Read the digest enzyme name
                XElement sampleEzymeElement = pepXMLReader.ReadFirstElement("sample_enzyme");
                XAttribute firstEnzymeName = pepXMLReader.ReadFirstAttribute(sampleEzymeElement, "name");
                if (null != firstEnzymeName)
                {
                    searchSummary += (String)firstEnzymeName;
                }

                searchSummary += " digest, ";

                //// Read the search engine name
                //IEnumerable<XElement> searchSummaryElements = pepXMLReader.ReadElements("search_summary");
                //XAttribute firstSearchEngine = pepXMLReader.ReadFirstAttribute(searchSummaryElements, "search_engine");
                //if (null != firstSearchEngine)
                //{
                //    searchSummary += (String)firstSearchEngine;
                //}
                // Read the search engine name
                XElement searchSummaryElement = pepXMLReader.ReadFirstElement("search_summary");
                XAttribute firstSearchEngine = pepXMLReader.ReadFirstAttribute(searchSummaryElement, "search_engine");
                if (null != firstSearchEngine)
                {
                    searchSummary += (String)firstSearchEngine;
                }

                searchSummary += " search engine, ";

                // Read the quantitation tool name, if there is one.
                searchSummary += "quantitation: ";
                String quantitationTool = "[none]";
                IEnumerable<XElement> analysisSummaryElements = pepXMLReader.ReadElements("analysis_summary").ToList();
                foreach (var element in analysisSummaryElements)
                {
                    XAttribute analysisAttribute = pepXMLReader.ReadFirstAttribute(element, "analysis");
                    if (null != analysisAttribute)
                    {
                        var analysis = (String)analysisAttribute;
                        if ((String.Empty != analysis) && IsQuantitationTool(analysis.ToLower()))
                        {
                            quantitationTool = analysis;
                        }
                    }
                }
                
                searchSummary += quantitationTool;
            }

            // Display the search summary
            searchSummaryLabel.Text = searchSummary;
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
