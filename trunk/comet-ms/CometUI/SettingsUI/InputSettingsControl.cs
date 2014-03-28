using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Windows.Forms;
using System.IO;
using CometUI.Properties;

namespace CometUI.SettingsUI
{
    public partial class InputSettingsControl : UserControl
    {
        private enum SearchType
        {
            SearchTypeTarget = 0,
            SearchTypeDecoyOne,
            SearchTypeDecoyTwo
            
        }
        private new Form  Parent { get; set; }

        public InputSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            comboBoxReadingFrame.Text = Settings.Default.NucleotideReadingFrame.ToString(CultureInfo.InvariantCulture);
            if (Settings.Default.IsProteinDB)
            {
                radioButtonProtein.Checked = true;
            }
            else
            {
                radioButtonNucleotide.Checked = true;
            }

            switch ((SearchType)Settings.Default.SearchType)
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

            textBoxDecoyPrefix.Text = Settings.Default.DecoyPrefix;
        }

        public string DatabaseFile 
        {
            get { return proteomeDbFileCombo.Text; }

            set { proteomeDbFileCombo.Text = value; }
        }

        private void BtnBrowseProteomeDbFileClick(object sender, EventArgs e)
        {
            string databaseFile = ShowOpenDatabaseFile();
            if (null != databaseFile)
            {
                DatabaseFile = databaseFile;
            }
        }

        private static string ShowOpenDatabaseFile()
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
    }
}
