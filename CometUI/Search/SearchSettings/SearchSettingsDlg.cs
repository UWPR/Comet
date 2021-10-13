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
using System.Windows.Forms;
using CometUI.Properties;
using System.Drawing;

namespace CometUI.Search.SearchSettings
{
    /// <summary>
    /// This is the main dialog window for search settings, accessed via:
    /// "Settings" menu --> "Search Settings..." menu item
    /// It hosts a tab control, and each tab on the control contains a group
    /// of search settings the user can modify.
    /// </summary>
    public partial class SearchSettingsDlg : Form
    {
        /// <summary>
        /// The controls hosted by the SearchSettingsDlg uses this
        /// property to notify the dialog that the user settings 
        /// have changed.
        /// </summary>
        public bool SettingsChanged { get; set; }
        
        /// <summary>
        /// The control that represents the "Input Settings" params.
        /// </summary>
        private InputSettingsControl InputSettingsControl { get; set; }

        /// <summary>
        /// The control that represents the "Output Settings" params.
        /// </summary>
        private OutputSettingsControl OutputSettingsControl { get; set; }

        /// <summary>
        /// The control that represents the "Enzyme Settings" params.
        /// </summary>
        private EnzymeSettingsControl EnzymeSettingsControl { get; set; }

        /// <summary>
        /// The control that represents the "Mass Settings" params.
        /// </summary>
        private MassSettingsControl MassSettingsControl { get; set; }

        /// <summary>
        /// The control that represents the "Static Mod Settings" params.
        /// </summary>
        private StaticModSettingsControl StaticModSettingsControl { get; set; }

        /// <summary>
        /// The control that represents the "Variable Mod Settings" params.
        /// </summary>
        private VarModSettingsControl VarModSettingsControl { get; set; }
        
        /// <summary>
        /// The control that represents the "Miscellaneous Settings" params.
        /// </summary>
        private MiscSettingsControl MiscSettingsControl { get; set; }
        
        /// <summary>
        /// Constructor for the SearchSettingsDlg that sets up the hosting
        /// of the various settings group tab page controls.
        /// </summary>
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

        private void SearchSettingsDlgLoad(object sender, EventArgs e)
        {
            EnzymeSettingsControl.Initialize();
            InputSettingsControl.Initialize();
            MassSettingsControl.Initialize();
            MiscSettingsControl.Initialize();
            OutputSettingsControl.Initialize();
            StaticModSettingsControl.Initialize();
            VarModSettingsControl.Initialize();
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

            if (!VarModSettingsControl.VerifyAndUpdateSettings())
            {
                MessageBox.Show(Resources.SearchSettingsDlg_BtnOKClick_Error_updating_var_mods_settings_,
                    Resources.SearchSettingsDlg_BtnOKClick_Search_Settings, MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                DialogResult = DialogResult.Abort;
            }

            if (!MiscSettingsControl.VerifyAndUpdateSettings())
            {
                MessageBox.Show(Resources.SearchSettingsDlg_BtnOKClick_Error_updating_misc_settings_,
                    Resources.SearchSettingsDlg_BtnOKClick_Search_Settings, MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                DialogResult = DialogResult.Abort;
            }

            DialogResult = DialogResult.OK;
        }
    }
}
