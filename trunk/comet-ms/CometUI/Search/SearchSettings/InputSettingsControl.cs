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
using System.Globalization;
using System.Windows.Forms;
using System.IO;
using CometUI.Properties;

namespace CometUI.Search.SearchSettings
{
    /// <summary>
    /// This class represents the tab page control for allowing the user to
    /// change the input settings search parameters, such as the FASTA file. 
    /// </summary>
    public partial class InputSettingsControl : UserControl
    {
        /// <summary>
        /// The FASTA database file to be used in the search.
        /// </summary>
        public string DatabaseFile
        {
            get { return proteomeDbFileCombo.Text; }

            set { proteomeDbFileCombo.Text = value; }
        }

        /// <summary>
        /// The type of search to run, e.g. target or decoy.
        /// </summary>
        private enum SearchType
        {
            SearchTypeTarget = 0,
            SearchTypeDecoyOne,
            SearchTypeDecoyTwo
            
        }

        private new SearchSettingsDlg  Parent { get; set; }

        /// <summary>
        /// Constructor for the input settings tab page.
        /// </summary>
        /// <param name="parent"> The tab control hosting this tab page. </param>
        public InputSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;
        }

        /// <summary>
        /// Public method to initialize the output settings tab page from the
        /// user's settings.
        /// </summary>
        public void Initialize()
        {
            InitializeFromDefaultSettings();
        }

        /// <summary>
        /// Allows users to browse to a FASTA file on their computer. 
        /// </summary>
        /// <returns> The full path to the file the user chose. </returns>
        public static string ShowOpenDatabaseFile()
        {
            const string filter = "All Files (*.*)|*.*";
            var fdlg = new OpenFileDialog
            {
                Title = Resources.InputFilesControl_ShowOpenDatabaseFile_Open_Proteome_Database_File,
                InitialDirectory = @".",
                Filter = filter,
                FilterIndex = 1,
                Multiselect = false,
                RestoreDirectory = true
            };

            if (fdlg.ShowDialog() == DialogResult.OK)
            {
                return fdlg.FileName;
            }

            return null;
        }

        /// <summary>
        /// Saves the settings the user modified to the user's settings.
        /// </summary>
        /// <returns> True if settings were updated successfully; False for error. </returns>
        public bool VerifyAndUpdateSettings()
        {
            if (!VerifyAndUpdateDatabaseFile())
            {
                return false;
            }

            if (!VerifyAndUpdateDatabaseType())
            {
                return false;
            }

            if (!VerifyAndUpdateSearchType())
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Initializes the input settings tab page from the settings saved
        /// in the user's settings.
        /// </summary>
        private void InitializeFromDefaultSettings()
        {
            proteomeDbFileCombo.Text = CometUI.SearchSettings.ProteomeDatabaseFile;

            comboBoxReadingFrame.Text = CometUI.SearchSettings.NucleotideReadingFrame.ToString(CultureInfo.InvariantCulture);
            if (CometUI.SearchSettings.IsProteinDB)
            {
                radioButtonProtein.Checked = true;
            }
            else
            {
                radioButtonNucleotide.Checked = true;
            }

            switch ((SearchType)CometUI.SearchSettings.SearchType)
            {
                case SearchType.SearchTypeDecoyOne:
                    radioButtonDecoyOne.Checked = true;
                    break;
                case SearchType.SearchTypeDecoyTwo:
                    radioButtonDecoyTwo.Checked = true;
                    break;
                case SearchType.SearchTypeTarget:
                    radioButtonTarget.Checked = true;
                    break;
                default:
                    radioButtonTarget.Checked = true;
                    break;
            }

            textBoxDecoyPrefix.Text = CometUI.SearchSettings.DecoyPrefix;
        }

        /// <summary>
        /// Ensures the database file specified by the user is valid, and
        /// stores the new path in the user's settings. 
        /// </summary>
        /// <returns> True for valid file; False for invalid file. </returns>
        private bool VerifyAndUpdateDatabaseFile()
        {
            if (!String.Equals(CometUI.SearchSettings.ProteomeDatabaseFile, proteomeDbFileCombo.Text))
            {
                if (String.Empty != proteomeDbFileCombo.Text)
                {
                    if (!File.Exists(proteomeDbFileCombo.Text))
                    {
                        String msg = "Proteome Database (.fasta) file " + proteomeDbFileCombo.Text + " does not exist.";
                        MessageBox.Show(msg, Resources.InputSettingsControl_VerifyAndSaveSettings_Search_Settings,
                                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return false;
                    }

                    CometUI.SearchSettings.ProteomeDatabaseFile = proteomeDbFileCombo.Text;
                    Parent.SettingsChanged = true;
                }
                else
                {
                    const string msg = "Search will not work without a valid proteome database file. Are you sure you want to clear this field?";
                    if (DialogResult.OK == MessageBox.Show(msg, Resources.InputSettingsControl_VerifyAndSaveSettings_Search_Settings,
                                    MessageBoxButtons.OKCancel, MessageBoxIcon.Warning))
                    {
                        CometUI.SearchSettings.ProteomeDatabaseFile = proteomeDbFileCombo.Text;
                        Parent.SettingsChanged = true;
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// Updates the database type (protein vs. nucleotide) in the 
        /// user's settings if changed.
        /// </summary>
        /// <returns> True for success</returns>
        private bool VerifyAndUpdateDatabaseType()
        {
            if (CometUI.SearchSettings.IsProteinDB != radioButtonProtein.Checked)
            {
                CometUI.SearchSettings.IsProteinDB = radioButtonProtein.Checked;
                Parent.SettingsChanged = true;
            }

            if (radioButtonNucleotide.Checked)
            {
                var nucleotideReadingFrame = Convert.ToInt32(comboBoxReadingFrame.SelectedItem);
                if (nucleotideReadingFrame != CometUI.SearchSettings.NucleotideReadingFrame)
                {
                    CometUI.SearchSettings.NucleotideReadingFrame = nucleotideReadingFrame;
                    Parent.SettingsChanged = true;
                }
            }

            return true;
        }

        /// <summary>
        /// Updates the search type (target vs. decoy) and decoy prefix in 
        /// the user's settings if changed.
        /// </summary>
        /// <returns> True for success</returns>
        private bool VerifyAndUpdateSearchType()
        {
            SearchType searchType;
            if (radioButtonDecoyOne.Checked)
            {
                searchType = SearchType.SearchTypeDecoyOne;
            }
            else if (radioButtonDecoyTwo.Checked)
            {
                searchType = SearchType.SearchTypeDecoyTwo;
            }
            else
            {
                searchType = SearchType.SearchTypeTarget;
            }

            if (searchType != (SearchType)CometUI.SearchSettings.SearchType)
            {
                CometUI.SearchSettings.SearchType = (int)searchType;
                Parent.SettingsChanged = true;
            }

            if ((radioButtonDecoyOne.Checked || radioButtonDecoyTwo.Checked) &&
                !textBoxDecoyPrefix.Text.Equals(CometUI.SearchSettings.DecoyPrefix))
            {
                CometUI.SearchSettings.DecoyPrefix = textBoxDecoyPrefix.Text;
                Parent.SettingsChanged = true;
            }

            return true;
        }

        private void BtnBrowseProteomeDbFileClick(object sender, EventArgs e)
        {
            string databaseFile = ShowOpenDatabaseFile();
            if (null != databaseFile)
            {
                DatabaseFile = databaseFile;
            }

            ProteomeDbFileNameChanged();
        }

        /// <summary>
        /// Stores a "history" or proteome database names in the drop-down
        /// for convenience.
        /// </summary>
        private void ProteomeDbFileNameChanged()
        {
            if (-1 == proteomeDbFileCombo.FindStringExact(proteomeDbFileCombo.Text))
            {
                proteomeDbFileCombo.Items.Add(proteomeDbFileCombo.Text);
            }
        }

        private void RadioButtonDecoyOneCheckedChanged(object sender, EventArgs e)
        {
            panelDecoyPrefix.Enabled = radioButtonDecoyOne.Checked;
        }

        private void RadioButtonDecoyTwoCheckedChanged(object sender, EventArgs e)
        {
            panelDecoyPrefix.Enabled = radioButtonDecoyTwo.Checked;
        }

        private void RadioButtonNucleotideCheckedChanged(object sender, EventArgs e)
        {
            panelNucleotideReadingFrame.Enabled = radioButtonNucleotide.Checked;
        }

        private void ProteomeDbFileComboSelectedIndexChanged(object sender, EventArgs e)
        {
            ProteomeDbFileNameChanged();
        }

        private void ProteomeDbFileComboTextUpdate(object sender, EventArgs e)
        {
            ProteomeDbFileNameChanged();
        }
    }
}
