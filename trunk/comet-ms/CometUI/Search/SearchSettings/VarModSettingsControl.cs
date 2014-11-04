using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;

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

        public StringCollection VarMods { get; set; }

        private new SearchSettingsDlg Parent { get; set; }
        public List<NamedVarMod> NamedVarModsList { get; set; }

        private const string AminoAcids = "GASPVTCLINDQKEMOHFRYW";
        private const int MaxNumVarMods = 9;


        public VarModSettingsControl(SearchSettingsDlg parent)
        {
            InitializeComponent();

            Parent = parent;

            NamedVarModsList = new List<NamedVarMod>();

            InitializeFromDefaultSettings();

            UpdateVarModsListBox();

            UpdateVarModsListBoxButtons();
        }

        public bool VerifyAndUpdateSettings()
        {
            //VarMods = VarModsDataGridViewToStringCollection();
            //for (int i = 0; i < VarMods.Count; i++ )
            //{
            //    if (!VarMods[i].Equals(CometUI.SearchSettings.VariableMods[i]))
            //    {
            //        Parent.SettingsChanged = true;
            //        CometUI.SearchSettings.VariableMods = VarMods;
            //        break;
            //    }
            //}

            //int maxModsInPeptide = (int)maxModsInPeptideTextBox.Value;
            //if (!maxModsInPeptide.Equals(CometUI.SearchSettings.MaxVarModsInPeptide))
            //{
            //    CometUI.SearchSettings.MaxVarModsInPeptide = maxModsInPeptide;
            //    Parent.SettingsChanged = true;
            //}

            return true;
        }

        public static bool IsValidAA(char aa)
        {
            return AminoAcids.Contains(aa.ToString(CultureInfo.InvariantCulture).ToUpper());
        }

        private void InitializeFromDefaultSettings()
        {
            VarMods = new StringCollection();
            int varModNum = 0;
            foreach (var item in CometUI.SearchSettings.VariableMods)
            {
                varModNum++;

                VarMods.Add(item);

                var varMod = CometParamsMap.GetVarModFromString(item);
                if (null != varMod)
                {
                    var varModName = GetVarModName(varMod);
                    NamedVarModsList.Add(new NamedVarMod(varModName, varMod));
                }
            }

            maxModsInPeptideTextBox.Text = CometUI.SearchSettings.MaxVarModsInPeptide.ToString(CultureInfo.InvariantCulture);            
        }

        public static String GetVarModName(VarMod varMod)
        {
            return varMod.VarModChar + " (" + varMod.VarModMass + ")";
        }

        private void UpdateVarModsListBox(int selectIndex = 0)
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
                varModsListBox.SelectedIndex = selectIndex;
            }

            varModsListBox.EndUpdate();
        }

        private void UpdateVarModsListBoxButtons()
        {
            addVarModBtn.Enabled = varModsListBox.Items.Count < MaxNumVarMods;
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
                    NamedVarModsList[i].VarModInfo = new VarMod(0.0, "X", 0, 3, -1, 0);
                    NamedVarModsList[i].Name = GetVarModName(NamedVarModsList[i].VarModInfo);
                    break;
                }
            }
            
            UpdateVarModsListBox();
        }

        private void AddVarModBtnClick(object sender, EventArgs e)
        {
            var varModInfoDlg = new VarModInfoDlg(this) {Title = "Add Variable Mod"};
            if ((DialogResult.OK == varModInfoDlg.ShowDialog()))
            {
                UpdateVarModsListBox();
                UpdateVarModsListBoxButtons();
            }
        }

        private void EditVarModBtnClick(object sender, EventArgs e)
        {
            var varModName = varModsListBox.SelectedItem.ToString();
            var varModInfoDlg = new VarModInfoDlg(this, varModName){Title = "Edit Variable Mod"};
            if ((DialogResult.OK == varModInfoDlg.ShowDialog()))
            {
                UpdateVarModsListBox();
                UpdateVarModsListBoxButtons();
            }
        }
    }
}
