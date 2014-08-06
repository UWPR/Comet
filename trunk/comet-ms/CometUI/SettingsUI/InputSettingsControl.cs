using System;
using System.Globalization;
using System.Windows.Forms;
using System.IO;
using CometUI.Properties;

namespace CometUI.SettingsUI
{
    public partial class InputSettingsControl : UserControl
    {
        public string DatabaseFile
        {
            get { return proteomeDbFileCombo.Text; }

            set { proteomeDbFileCombo.Text = value; }
        }

        private enum SearchType
        {
            SearchTypeTarget = 0,
            SearchTypeDecoyOne,
            SearchTypeDecoyTwo
            
        }
        private new SearchSettingsDlg  Parent { get; set; }

        public InputSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
        }

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

        public bool VerifyAndUpdateSettings()
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

        private void BtnBrowseProteomeDbFileClick(object sender, EventArgs e)
        {
            string databaseFile = ShowOpenDatabaseFile();
            if (null != databaseFile)
            {
                DatabaseFile = databaseFile;
            }

            ProteomeDbFileNameChanged();
        }

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
