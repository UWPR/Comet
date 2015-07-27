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
using System.Collections.Generic;
using System.Globalization;
using System.Windows.Forms;
using System.Collections.Specialized;

namespace CometUI.Search.SearchSettings
{
    /// <summary>
    /// This class represents the tab page control for allowing the user to
    /// change the enzyme settings search parameters. 
    /// </summary>
    public partial class EnzymeSettingsControl : UserControl
    {
        public StringCollection EnzymeInfo { get; set; }

        private int SearchEnzymeComboEditListIndex { get; set; }
        private int SampleEnzymeComboEditListIndex { get; set; }
        private int SearchEnzymeCurrentSelectedIndex { get; set; }
        private int SampleEnzymeCurrentSelectedIndex { get; set; }

        private new SearchSettingsDlg Parent { get; set; }
        private EnzymeInfoDlg EnzymeInfoDlg { get; set; }
        private readonly Dictionary<int, string> _enzymeTermini = new Dictionary<int, string>();

        /// <summary>
        /// Constructor for the enzyme settings tab page.
        /// </summary>
        /// <param name="parent"> The tab control hosting this tab page. </param>
        public EnzymeSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;

            _enzymeTermini.Add(2, "Fully-digested");
            _enzymeTermini.Add(1, "Semi-digested");
            _enzymeTermini.Add(8, "N-term");
            _enzymeTermini.Add(9, "C-term");

            Dictionary<int, string>.KeyCollection enzymeTerminiKeys = _enzymeTermini.Keys;
            foreach (var key in enzymeTerminiKeys)
            {
                enzymeTerminiCombo.Items.Add(_enzymeTermini[key]);
            }
        }

        /// <summary>
        /// Public method to initialize the output settings tab page from the
        /// user's settings.
        /// </summary>
        public void Initialize()
        {
            InitializeFromDefaultSettings();

            EnzymeInfoDlg = new EnzymeInfoDlg(this);
        }

        /// <summary>
        /// Saves the enzyme settings the user modified to the user's settings.
        /// </summary>
        /// <returns> True if settings were updated successfully; False for error. </returns>
        public bool VerifyAndUpdateSettings()
        {
            if (CometUIMainForm.SearchSettings.SearchEnzymeNumber != SearchEnzymeCurrentSelectedIndex)
            {
                CometUIMainForm.SearchSettings.SearchEnzymeNumber = SearchEnzymeCurrentSelectedIndex;
                Parent.SettingsChanged = true;
            }

            if (CometUIMainForm.SearchSettings.SampleEnzymeNumber != SampleEnzymeCurrentSelectedIndex)
            {
                CometUIMainForm.SearchSettings.SampleEnzymeNumber = SampleEnzymeCurrentSelectedIndex;
                Parent.SettingsChanged = true;
            }

            var allowedMissedCleavages = missedCleavagesCombo.SelectedIndex;
            if (CometUIMainForm.SearchSettings.AllowedMissedCleavages != allowedMissedCleavages)
            {
                CometUIMainForm.SearchSettings.AllowedMissedCleavages = allowedMissedCleavages;
                Parent.SettingsChanged = true;
            }

            // Check each key in the dictionary to see which matches the value
            // currently selected in the enzyme termini combo box.
            foreach (var key in _enzymeTermini.Keys)
            {
                if (_enzymeTermini[key].Equals(enzymeTerminiCombo.SelectedItem.ToString()))
                {
                    if (CometUIMainForm.SearchSettings.EnzymeTermini != key)
                    {
                        CometUIMainForm.SearchSettings.EnzymeTermini = key;
                        Parent.SettingsChanged = true;
                    }

                    break;
                }
            }

            if (EnzymeInfoDlg.EnzymeInfoChanged)
            {
                CometUIMainForm.SearchSettings.EnzymeInfo = EnzymeInfo;
                Parent.SettingsChanged = true;
            }

            var digestMassRangeMin = (double) digestMassRangeMinTextBox.DecimalValue;
            if (!digestMassRangeMin.Equals(CometUIMainForm.SearchSettings.digestMassRangeMin))
            {
                CometUIMainForm.SearchSettings.digestMassRangeMin = digestMassRangeMin;
                Parent.SettingsChanged = true;
            }

            var digestMassRangeMax = (double)digestMassRangeMaxTextBox.DecimalValue;
            if (!digestMassRangeMax.Equals(CometUIMainForm.SearchSettings.digestMassRangeMax))
            {
                CometUIMainForm.SearchSettings.digestMassRangeMax = digestMassRangeMax;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        /// <summary>
        /// Initializes the input settings tab page from the settings saved
        /// in the user's settings.
        /// </summary>
        private void InitializeFromDefaultSettings()
        {
            enzymeTerminiCombo.SelectedItem = _enzymeTermini[CometUIMainForm.SearchSettings.EnzymeTermini];

            // For this particular combo, index == value of allowed missed cleavages
            missedCleavagesCombo.SelectedItem = CometUIMainForm.SearchSettings.AllowedMissedCleavages.ToString(CultureInfo.InvariantCulture);


            EnzymeInfo = new StringCollection();
            foreach (var item in CometUIMainForm.SearchSettings.EnzymeInfo)
            {
                EnzymeInfo.Add(item);
            }

            UpdateEnzymeInfo();

            SearchEnzymeCurrentSelectedIndex = CometUIMainForm.SearchSettings.SearchEnzymeNumber;
            SampleEnzymeCurrentSelectedIndex = CometUIMainForm.SearchSettings.SampleEnzymeNumber;

            searchEnzymeCombo.SelectedIndex = SearchEnzymeCurrentSelectedIndex;
            sampleEnzymeCombo.SelectedIndex = SampleEnzymeCurrentSelectedIndex;

            digestMassRangeMinTextBox.Text = 
                CometUIMainForm.SearchSettings.digestMassRangeMin.ToString(CultureInfo.InvariantCulture);
            digestMassRangeMaxTextBox.Text = 
                CometUIMainForm.SearchSettings.digestMassRangeMax.ToString(CultureInfo.InvariantCulture);
        }

        private void UpdateEnzymeInfo()
        {
            sampleEnzymeCombo.Items.Clear();
            searchEnzymeCombo.Items.Clear();

            foreach (var row in EnzymeInfo)
            {
                string[] cells = row.Split(',');

                String sampleEnzymeItem = cells[1] + " (" + cells[3] + "/" + cells[4] + ")";
                sampleEnzymeCombo.Items.Add(sampleEnzymeItem);

                String searchEnzymeItem = cells[1] + " (" + cells[3] + "/" + cells[4] + ")";
                searchEnzymeCombo.Items.Add(searchEnzymeItem);
            }

            // Add the "Edit List" item at the end of the lists
            searchEnzymeCombo.Items.Add("<Edit List...>");
            SearchEnzymeComboEditListIndex = searchEnzymeCombo.Items.Count - 1;
            sampleEnzymeCombo.Items.Add("<Edit List...>");
            SampleEnzymeComboEditListIndex = sampleEnzymeCombo.Items.Count - 1;
        }

        private void SearchEnzymeComboSelectedIndexChanged(object sender, EventArgs e)
        {
            var srchEnzymeCombo = (ComboBox) sender;
            if (SearchEnzymeComboEditListIndex == srchEnzymeCombo.SelectedIndex)
            {
                if ((DialogResult.OK == EnzymeInfoDlg.ShowDialog()) &&
                    EnzymeInfoDlg.EnzymeInfoChanged)
                {
                    UpdateEnzymeInfo();
                }
                else
                {
                    srchEnzymeCombo.SelectedIndex = SearchEnzymeCurrentSelectedIndex;
                }
            }
            else
            {
                SearchEnzymeCurrentSelectedIndex = srchEnzymeCombo.SelectedIndex;
            }
        }

        private void SampleEnzymeComboSelectedIndexChanged(object sender, EventArgs e)
        {
            var smplEnzymeCombo = (ComboBox)sender;
            if (SampleEnzymeComboEditListIndex == smplEnzymeCombo.SelectedIndex)
            {
                if (DialogResult.OK == EnzymeInfoDlg.ShowDialog())
                {
                    if (EnzymeInfoDlg.EnzymeInfoChanged)
                    {
                        UpdateEnzymeInfo();
                    }
                }
                else
                {
                    smplEnzymeCombo.SelectedIndex = SampleEnzymeCurrentSelectedIndex;
                }
            }
            else
            {
                SampleEnzymeCurrentSelectedIndex = smplEnzymeCombo.SelectedIndex;
            }
        }
    }
}
