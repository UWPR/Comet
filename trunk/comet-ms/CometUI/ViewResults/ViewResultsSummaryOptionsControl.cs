/*
   Copyright 2015 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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

        public String PepXMLFile
        {
            set { pepXMLFileCombo.Text = value; }
        }
        
        public String DecoyPrefix
        {
            get
            {
                return customDecoyPrefixCheckBox.Checked ? textBoxCustomDecoyPrefix.Text : CometUIMainForm.SearchSettings.DecoyPrefix;
            }
        }

        public String ResultsListSummaryText
        {
            set { resultsListSummaryLabel.Text = value; }
        }

        private static readonly string[] QuantitationTools = new[] {"xpress","asapratio","libra"};

        private ViewSearchResultsControl ViewSearchResultsControl { get; set; }


        public ViewResultsSummaryOptionsControl(ViewSearchResultsControl parent)
        {
            InitializeComponent();

            customDecoyPrefixCheckBox.Checked = false;
            
            ViewSearchResultsControl = parent;
        }

        private void BtnBrowsePepXMLFileClick(object sender, EventArgs e)
        {
            var pepXMLFile = CometUIMainForm.ShowOpenPepXMLFile();
            if (null != pepXMLFile)
            {
                pepXMLFileCombo.Text = pepXMLFile;
            }
        }

        public bool UpdateSummaryOptions(String resultsPepXMLFile, PepXMLReader pepXMLReader)
        {
            ErrorMessage = String.Empty;
            UpdatePepXMLFileCombo(resultsPepXMLFile);

            if (!UpdateSearchSummaryLabel(pepXMLReader))
            {
                return false;
            }

            return true;
        }

        private void UpdatePepXMLFileCombo(String resultsPepXMLFile)
        {
            pepXMLFileCombo.Text = resultsPepXMLFile;
        }

        private bool UpdateSearchSummaryLabel(PepXMLReader pepXMLReader)
        {
            if (null == pepXMLReader)
            {
                // If there is no results file, clear the search summary display
                searchResultsSummaryLabel.Text = String.Empty;
                resultsListSummaryLabel.Text = String.Empty;
                return true;
            }

            var searchSummary = String.Empty;
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
            ViewSearchResultsControl.UpdateViewSearchResults(pepXMLFileCombo.Text, DecoyPrefix);
        }

        private void CustomDecoyPrefixCheckBoxCheckedChanged(object sender, EventArgs e)
        {
            textBoxCustomDecoyPrefix.Enabled = customDecoyPrefixCheckBox.Checked;
        }

        private void eValueCheckBox_CheckedChanged(object sender, EventArgs e)
        {
           textBoxEValueCutoff.Enabled = eValueCheckBox.Checked;
        }

        private void textBoxCustomDecoyPrefix_TextChanged(object sender, EventArgs e)
        {

        }
    }
}
