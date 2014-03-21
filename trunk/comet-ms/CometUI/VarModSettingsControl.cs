using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI
{
    public partial class VarModSettingsControl : UserControl
    {
        public StringCollection VarMods { get; set; }
        private new Form Parent { get; set; }

        public VarModSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;
                 
            InitializeFromDefaultSettings();

            UpdateVarModsDataGridView();
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

        private void UpdateVarModsDataGridView()
        {
            varModsDataGridView.Rows.Add(VarMods.Count);
            for (int rowIndex = 0; rowIndex < VarMods.Count; rowIndex++)
            {
                var varModsRow = VarMods[rowIndex];
                string[] varModsCells = varModsRow.Split(',');
                var dataGridViewRow = varModsDataGridView.Rows[rowIndex];
                for (int colIndex = 0; colIndex < dataGridViewRow.Cells.Count; colIndex++)
                {
                    string cellColTitle = dataGridViewRow.Cells[colIndex].OwningColumn.HeaderCell.Value.ToString();
                    if (cellColTitle.Equals("Binary Mod"))
                    {
                        var checkBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewCheckBoxCell;
                        if (null != checkBoxCell)
                        {
                            checkBoxCell.Value = varModsCells[colIndex].Equals("1");
                        }
                    }
                    else
                    {
                        var textBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewTextBoxCell;
                        if (null != textBoxCell)
                        {
                            textBoxCell.Value = varModsCells[colIndex];
                        }
                    }
                }
            }
        }
    }
}
