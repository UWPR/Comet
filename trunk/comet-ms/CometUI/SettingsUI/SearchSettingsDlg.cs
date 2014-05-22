using System;
using System.Collections.Specialized;
using System.Windows.Forms;
using CometUI.Properties;
using System.Drawing;

namespace CometUI.SettingsUI
{
    public partial class SearchSettingsDlg : Form
    {
        public bool SettingsChanged { get; set; }
        private InputSettingsControl InputSettingsControl { get; set; }
        private OutputSettingsControl OutputSettingsControl { get; set; }
        private EnzymeSettingsControl EnzymeSettingsControl { get; set; }
        private MassSettingsControl MassSettingsControl { get; set; }
        private StaticModSettingsControl StaticModSettingsControl { get; set; }
        private VarModSettingsControl VarModSettingsControl { get; set; }
        private MiscSettingsControl MiscSettingsControl { get; set; }
        
        public SearchSettingsDlg()
        {
            InitializeComponent();

            SettingsChanged = false;

            // Create and add Input Files tab page
            InputSettingsControl = new InputSettingsControl(this)
            {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            inputFilesTabPage.Controls.Add(InputSettingsControl);

            // Create and add Input Files tab page
            OutputSettingsControl = new OutputSettingsControl(this)
            {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            outputTabPage.Controls.Add(OutputSettingsControl);

            // Create and add Enzyme tab page
            EnzymeSettingsControl = new EnzymeSettingsControl(this)
            {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            enzymeTabPage.Controls.Add(EnzymeSettingsControl);

            // Create and add Mass tab page
            MassSettingsControl = new MassSettingsControl(this)
            {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            massesTabPage.Controls.Add(MassSettingsControl);

            // Create and add Mods tab page
            StaticModSettingsControl = new StaticModSettingsControl(this)
            {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            staticModsTabPage.Controls.Add(StaticModSettingsControl);
    
            // Create and add Mods tab page
            VarModSettingsControl = new VarModSettingsControl(this)
            {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            varModsTabPage.Controls.Add(VarModSettingsControl);

            // Create and add Misc tab page
            MiscSettingsControl = new MiscSettingsControl(this)
            {
                Anchor = (AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right),
                Location = new Point(0, 0)
            };
            miscTabPage.Controls.Add(MiscSettingsControl);
        }

        public static bool ConvertStrToDouble(string strValue, out double doubleValueOut)
        {
            var doubleValue = 0.0;
            try
            {
                doubleValue = Convert.ToDouble(strValue);
            }
            catch (Exception)
            {
                return false;
            }
            finally
            {
                doubleValueOut = doubleValue;
            }

            return true;
        }

        public static void DataGridViewToStringCollection(DataGridView dataGridView, out StringCollection strCollection)
        {
            strCollection = new StringCollection();
            for (int rowIndex = 0; rowIndex < dataGridView.Rows.Count; rowIndex++)
            {
                var dataGridViewRow = dataGridView.Rows[rowIndex];
                string row = String.Empty;
                for (int colIndex = 0; colIndex < dataGridViewRow.Cells.Count; colIndex++)
                {
                    var textBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewTextBoxCell;
                    if (null != textBoxCell)
                    {
                        row += textBoxCell.Value;
                        if (colIndex != dataGridViewRow.Cells.Count - 1)
                        {
                            row += ",";
                        }
                    }
                }
                strCollection.Add(row);
            }
        }

        private void BtnCancelClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
        }

        private void BtnOKClick(object sender, EventArgs e)
        {
            if (!InputSettingsControl.VerifyAndUpdateSettings())
            {
                MessageBox.Show(Resources.SearchSettingsDlg_BtnOKClick_Error_updating_input_settings_, Resources.SearchSettingsDlg_BtnOKClick_Search_Settings, MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                DialogResult = DialogResult.Abort;
            }

            if (!OutputSettingsControl.VerifyAndUpdateSettings())
            {
                MessageBox.Show(Resources.SearchSettingsDlg_BtnOKClick_Error_updating_Output_settings_, Resources.SearchSettingsDlg_BtnOKClick_Search_Settings, MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                DialogResult = DialogResult.Abort;
            }

            if (!EnzymeSettingsControl.VerifyAndUpdateSettings())
            {
                MessageBox.Show(Resources.SearchSettingsDlg_BtnOKClick_Error_updating_enzyme_settings_,
                                Resources.SearchSettingsDlg_BtnOKClick_Search_Settings, MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                DialogResult = DialogResult.Abort;
            }

            if (!MassSettingsControl.VerifyAndUpdateSettings())
            {
                MessageBox.Show(Resources.SearchSettingsDlg_BtnOKClick_Error_updating_mass_settings_,
                    Resources.SearchSettingsDlg_BtnOKClick_Search_Settings, MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                DialogResult = DialogResult.Abort;
            }

            if (!StaticModSettingsControl.VerifyAndUpdateSettings())
            {
                MessageBox.Show(Resources.SearchSettingsDlg_BtnOKClick_Error_updating_static_mods_settings_,
                    Resources.SearchSettingsDlg_BtnOKClick_Search_Settings, MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                DialogResult = DialogResult.Abort;
            }

            DialogResult = DialogResult.OK;
        }

    }
}
