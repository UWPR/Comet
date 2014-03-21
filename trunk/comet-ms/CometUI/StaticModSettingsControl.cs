using System.Collections.Specialized;
using System.Globalization;
using System.Windows.Forms;
using CometUI.Properties;

namespace CometUI
{
    public partial class StaticModSettingsControl : UserControl
    {
        private new Form Parent { get; set; }
        public StringCollection StaticMods { get; set; }

        public StaticModSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;
                 
            InitializeFromDefaultSettings();

            UpdateStatidModsDataGridView();
        }

        private void InitializeFromDefaultSettings()
        {
            StaticMods = new StringCollection();
            foreach (var item in Settings.Default.StaticMods)
            {
                StaticMods.Add(item);
            }

            staticNTermPeptideTextBox.Text = Settings.Default.StaticModNTermPeptide.ToString(CultureInfo.InvariantCulture);
            staticCTermPeptideTextBox.Text = Settings.Default.StaticModCTermPeptide.ToString(CultureInfo.InvariantCulture);
            staticNTermProteinTextBox.Text = Settings.Default.StaticModNTermProtein.ToString(CultureInfo.InvariantCulture);
            staticCTermProteinTextBox.Text = Settings.Default.StaticModCTermProtein.ToString(CultureInfo.InvariantCulture);
        }

        private void UpdateStatidModsDataGridView()
        {
            staticModsDataGridView.Rows.Add(StaticMods.Count);
            for (int rowIndex = 0; rowIndex < StaticMods.Count; rowIndex++)
            {
                var staticModsRow = StaticMods[rowIndex];
                string[] staticModsCells = staticModsRow.Split(',');
                var dataGridViewRow = staticModsDataGridView.Rows[rowIndex];
                for (int colIndex = 0; colIndex < dataGridViewRow.Cells.Count; colIndex++)
                {
                    var textBoxCell = dataGridViewRow.Cells[colIndex] as DataGridViewTextBoxCell;
                    if (null != textBoxCell)
                    {
                        textBoxCell.Value = staticModsCells[colIndex];
                    }
                }
            }
            
        }
    }
}
