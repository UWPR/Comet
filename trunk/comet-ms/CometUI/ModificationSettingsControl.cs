using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI
{
    public partial class ModificationSettingsControl : UserControl
    {
        public StringCollection VarMods { get; set; }
        private new Form Parent { get; set; }

        public List<StaticMod> StaticMods { get; set; }


        public ModificationSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;
                 
            InitializeFromDefaultSettings();

            foreach (var row in VarMods)
            {
                string[] cells = row.Split(',');
                varModsDataGridView.Rows.Add(cells);
            }

            StaticMods = new List<StaticMod>();
            StaticMods.Add(new StaticMod("Glycine", "G", 57.0513, 57.02146));
            StaticMods.Add(new StaticMod("Alanine", "A", 71.0779, 71.03711));
            StaticMods.Add(new StaticMod("Serine", "S", 87.0773, 87.03203));
            StaticMods.Add(new StaticMod("Proline", "P", 97.1152, 97.05276));
            StaticMods.Add(new StaticMod("Valine", "V", 99.1311, 99.06841));
            StaticMods.Add(new StaticMod("Threonine", "T", 101.1038, 101.04768));
            StaticMods.Add(new StaticMod("Cysteine", "C", 103.1429, 103.00918));
            StaticMods.Add(new StaticMod("Leucine", "L", 113.1576, 113.08406));
            StaticMods.Add(new StaticMod("Isoleucine", "I", 113.1576, 113.08406));
            StaticMods.Add(new StaticMod("Asparagine", "N", 114.1026, 114.04293));
            StaticMods.Add(new StaticMod("Aspartic Acid", "D", 115.0874, 115.02694));
            StaticMods.Add(new StaticMod("Glutamine", "Q", 128.1292, 128.05858));
            StaticMods.Add(new StaticMod("Lysine", "K", 128.1723, 128.09496));
            StaticMods.Add(new StaticMod("Glutamic Acid", "E", 129.1140, 129.04259));
            StaticMods.Add(new StaticMod("Methionine", "M", 131.1961, 131.04048));
            StaticMods.Add(new StaticMod("Ornithine", "O", 132.1610, 132.08988));
            StaticMods.Add(new StaticMod("Histidine", "H", 137.1393, 137.05891));
            StaticMods.Add(new StaticMod("Phenylalanine", "F", 147.1739, 147.06841));
            StaticMods.Add(new StaticMod("Arginine", "R", 156.1857, 156.10111));
            StaticMods.Add(new StaticMod("Tyrosine", "Y", 163.0633, 163.06333));
            StaticMods.Add(new StaticMod("Tryptophan", "W", 186.0793, 186.07931));
            StaticMods.Add(new StaticMod("User Amino Acid", "B", 0.0000, 0.0000));
            StaticMods.Add(new StaticMod("User Amino Acid", "J", 0.0000, 0.0000));
            StaticMods.Add(new StaticMod("User Amino Acid", "U", 0.0000, 0.0000));
            StaticMods.Add(new StaticMod("User Amino Acid", "X", 0.0000, 0.0000));
            StaticMods.Add(new StaticMod("User Amino Acid", "Z", 0.0000, 0.0000));

            foreach (StaticMod mod in StaticMods)
            {
                staticModsList.Items.Add(mod.Name + " (" + mod.Residue + ")");
            }

            staticModsList.SelectedIndex = 0;
        }

        private void InitializeFromDefaultSettings()
        {
            VarMods = new StringCollection();
            foreach (var item in Settings.Default.VariableMods)
            {
                VarMods.Add(item);
            }

            variableNTerminusTextBox.Text = Settings.Default.VariableNTerminus.ToString(CultureInfo.InvariantCulture);
            variableCTerminusTextBox.Text = Settings.Default.VariableCTerminus.ToString(CultureInfo.InvariantCulture);
            variableNTerminusDistTextBox.Text = Settings.Default.VariableNTermDistance.ToString(CultureInfo.InvariantCulture);
            variableCTerminusDistTextBox.Text = Settings.Default.VariableCTermDistance.ToString(CultureInfo.InvariantCulture);

            maxModsInPeptideTextBox.Text = Settings.Default.MaxVarModsInPeptide.ToString(CultureInfo.InvariantCulture);            
        }

        private void EditStaticModButtonClick(object sender, System.EventArgs e)
        {
            var dlgEditStaticMod = new EditStaticModDlg(StaticMods[staticModsList.SelectedIndex]);
            if ((DialogResult.OK == dlgEditStaticMod.ShowDialog()))
            {
                if (dlgEditStaticMod.MonoMassChanged)
                {
                    StaticMods[staticModsList.SelectedIndex].MonoisotopicMass = dlgEditStaticMod.MonoMass;
                }

                if (dlgEditStaticMod.AvgMassChanged)
                {
                    StaticMods[staticModsList.SelectedIndex].AvgMass = dlgEditStaticMod.AvgMass;
                }
            }
        }
    }

    public class StaticMod
    {
        public String Name { get; set; }
        public String Residue { get; set; }
        public double AvgMass { get; set; }
        public double MonoisotopicMass { get; set; }

        public StaticMod(String name, String residue, double avgMass, double monoMass)
        {
            Name = name;
            Residue = residue;
            AvgMass = avgMass;
            MonoisotopicMass = monoMass;
        }
    }

}
