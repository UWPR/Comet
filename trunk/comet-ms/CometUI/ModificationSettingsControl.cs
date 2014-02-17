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

        public List<Modification> StaticMods { get; set; }


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

            StaticMods = new List<Modification>();
            StaticMods.Add(new Modification("Glycine(G)", "G", 57.0513, 57.02146));
            StaticMods.Add(new Modification("Alanine(A)", "A", 71.0779, 71.03711));
            StaticMods.Add(new Modification("Serine(S)", "S", 87.0773, 87.03203));
            StaticMods.Add(new Modification("Proline(P)", "P", 97.1152, 97.05276));
            StaticMods.Add(new Modification("User Amino Acid (B)", "B", 0.0000, 0.0000));
            StaticMods.Add(new Modification("User Amino Acid (J)", "J", 0.0000, 0.0000));
            StaticMods.Add(new Modification("User Amino Acid (U)", "U", 0.0000, 0.0000));
            StaticMods.Add(new Modification("User Amino Acid (X)", "X", 0.0000, 0.0000));
            StaticMods.Add(new Modification("User Amino Acid (Z)", "Z", 0.0000, 0.0000));

            foreach (Modification mod in StaticMods)
            {
                staticModsList.Items.Add(mod.Name);
            }

            staticModsList.SelectedIndex = 0;

//Glycine (G)
//Alanine (A)
//Serine (S)
//Proline (P)
//Valine (V)
//Threonine (T)
//Cysteine (C)
//Leucine (L)
//Isoleucine (I)
//Asparagine (N)
//Aspartic Acid (D)
//Glutamine (Q)
//Lysine (K)
//Glutamic Acid (E)
//Methionine (M)
//Ornithine (O)
//Histidine (H)
//Phenylalanine (F)
//Arginine (R)
//Tyrosine (Y)
//Tryptophan (W)
//User Amino Acid (B)
//User Amino Acid (J)
//User Amino Acid (U)
//User Amino Acid (X)
//User Amino Acid (Z)
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
                
            }
        }
    }

    public class Modification
    {
        public String Name { get; set; }
        public String Residue { get; set; }
        public double AvgMass { get; set; }
        public double MonoisotopicMass { get; set; }

        public Modification(String name, String residue, double avgMass, double monoMass)
        {
            Name = name;
            Residue = residue;
            AvgMass = avgMass;
            MonoisotopicMass = monoMass;
        }
    }

}
