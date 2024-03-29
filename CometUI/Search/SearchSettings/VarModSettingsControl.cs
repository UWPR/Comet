﻿/*
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
using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI.Search.SearchSettings
{
    public partial class VarModSettingsControl : UserControl
    {
        public class NamedVarMod
        {
            public String Name { get; set; }
            public VarMod VarModInfo { get; set; }

            public NamedVarMod(String displayName, VarMod varModInfo)
            {
                Name = displayName;
                VarModInfo = varModInfo;
            }
        }

        public List<NamedVarMod> NamedVarModsList { get; set; }
        public const string AminoAcids = "GASPVTCLINDQKEMOHFRYW";

        private new SearchSettingsDlg Parent { get; set; }


        public VarModSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;
        }

        public void Initialize()
        {
            NamedVarModsList = new List<NamedVarMod>();
            InitializeFromDefaultSettings();
            UpdateVarModsListBox();
            UpdateVarModsListBoxButtons();
        }

        public bool VerifyAndUpdateSettings()
        {
            VerifyAndUpdateVarModsList();
            VerifyAndUpdateMaxModsInPeptide();
            VerifyAndUpdateRequireVarMod();
            return true;
        }

        private void VerifyAndUpdateVarModsList()
        {
            var varModsChanged = false;
            var varModsStrCollection = new StringCollection();
            for (int i = 0; i < NamedVarModsList.Count; i++)
            {
                String varModInfoStr = GetVarModStr(NamedVarModsList[i].VarModInfo);
                varModsStrCollection.Add(varModInfoStr);
                if (!varModInfoStr.Equals(CometUIMainForm.SearchSettings.VariableMods[i]))
                {
                    varModsChanged = true;
                }
            }

            if (varModsChanged)
            {
                Parent.SettingsChanged = true;
                CometUIMainForm.SearchSettings.VariableMods = varModsStrCollection;
            }
        }

        private void VerifyAndUpdateMaxModsInPeptide()
        {
            var maxModsInPeptide = (int)maxModsInPeptideTextBox.Value;
            if (!maxModsInPeptide.Equals(CometUIMainForm.SearchSettings.MaxVarModsInPeptide))
            {
                CometUIMainForm.SearchSettings.MaxVarModsInPeptide = maxModsInPeptide;
                Parent.SettingsChanged = true;
            }
        }

        private void VerifyAndUpdateRequireVarMod()
        {
            var requireVarMod = requireVarModCheckBox.Checked;
            if (requireVarMod != CometUIMainForm.SearchSettings.RequireVariableMod)
            {
                CometUIMainForm.SearchSettings.RequireVariableMod = requireVarMod;
                Parent.SettingsChanged = true;
            }
        }

        private String GetVarModStr(VarMod varMod)
        {
            String varModStr = varMod.VarModMass + ","
                               + varMod.VarModChar + ","
                               + varMod.BinaryMod + ","
                               + varMod.MaxNumVarModAAPerMod + ","
                               + varMod.VarModTermDistance + ","
                               + varMod.WhichTerm + ","
                               + varMod.RequireThisMod;
            return varModStr;
        }

        public static bool IsValidAA(char aa)
        {
            return AminoAcids.Contains(aa.ToString(CultureInfo.InvariantCulture).ToUpper());
        }

        public static String GetVarModName(VarMod varMod)
        {
            return varMod.VarModChar + " (" + varMod.VarModMass + ")";
        }

        private void InitializeFromDefaultSettings()
        {
            foreach (var item in CometUIMainForm.SearchSettings.VariableMods)
            {
                var varMod = CometParamsMap.GetVarModFromString(item);
                if (null != varMod)
                {
                    var varModName = GetVarModName(varMod);
                    NamedVarModsList.Add(new NamedVarMod(varModName, varMod));
                }
            }

            maxModsInPeptideTextBox.Text = CometUIMainForm.SearchSettings.MaxVarModsInPeptide.ToString(CultureInfo.InvariantCulture);

            requireVarModCheckBox.Checked = CometUIMainForm.SearchSettings.RequireVariableMod;
        }

        private void UpdateVarModsListBox(String selectedItem = null)
        {
            varModsListBox.BeginUpdate();
            varModsListBox.Items.Clear();
            foreach (var item in NamedVarModsList)
            {
                if (IsValidResidue(item.VarModInfo.VarModChar))
                {
                    varModsListBox.Items.Add(item.Name);
                }
            }

            if (varModsListBox.Items.Count > 0)
            {
                if (null != selectedItem)
                {
                    varModsListBox.SelectedItem = selectedItem;
                }
                else
                {
                    varModsListBox.SelectedIndex = 0;
                }
            }

            varModsListBox.EndUpdate();

            UpdateVarModsListBoxButtons();
        }

        private void UpdateVarModsListBoxButtons()
        {
            addVarModBtn.Enabled = varModsListBox.Items.Count < CometParamsMap.MaxNumVarMods;
            editVarModBtn.Enabled = -1 != varModsListBox.SelectedIndex;
            removeVarModBtn.Enabled = editVarModBtn.Enabled;
        }

        private static bool IsValidResidue(String residue)
        {
            foreach (var character in residue)
            {
                if (!AminoAcids.Contains(character.ToString(CultureInfo.InvariantCulture).ToUpper()))
                {
                    return false;
                }
            }

            return true;
        }

        private void RemoveVarModBtnClick(object sender, EventArgs e)
        {
            RemoveVarMod();
        }

        private void RemoveVarMod()
        {
            var modName = varModsListBox.SelectedItem;
            for (int i = 0; i < NamedVarModsList.Count; i++)
            {
                if (modName.Equals(NamedVarModsList[i].Name))
                {
                    NamedVarModsList[i].VarModInfo = new VarMod(0.0, "X", 0, 3, -1, 0, 0);
                    NamedVarModsList[i].Name = GetVarModName(NamedVarModsList[i].VarModInfo);
                    break;
                }
            }
            
            UpdateVarModsListBox();
        }

        private void AddVarModBtnClick(object sender, EventArgs e)
        {
            var varModInfoDlg = new VarModInfoDlg(this) {Title = Resources.VarModSettingsControl_AddVarModBtnClick_Add_Variable_Mod};
            if ((DialogResult.OK == varModInfoDlg.ShowDialog()))
            {
                var selectedItem = NamedVarModsList[varModInfoDlg.NamedVarModsListIndex].Name;
                UpdateVarModsListBox(selectedItem);
                if (varModsListBox.Items.Count >= CometParamsMap.MaxNumVarMods)
                {
                    MessageBox.Show(Resources.VarModSettingsControl_AddVarModBtnClick_You_have_reached_the_maximum_number_of_variable_mods_allowed_, 
                        Resources.VarModSettingsControl_AddVarModBtnClick_Add_Variable_Mod,
                        MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
        }

        private void EditVarModBtnClick(object sender, EventArgs e)
        {
            String selectedItem = varModsListBox.SelectedItem.ToString();
            var varModName = varModsListBox.SelectedItem.ToString();
            var varModInfoDlg = new VarModInfoDlg(this, varModName){Title = "Edit Variable Mod"};
            if ((DialogResult.OK == varModInfoDlg.ShowDialog()))
            {
                UpdateVarModsListBox(selectedItem);
            }
        }
    }
}
