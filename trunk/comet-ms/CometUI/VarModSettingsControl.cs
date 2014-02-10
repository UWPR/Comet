using System.Collections.Specialized;
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

            foreach (var row in VarMods)
            {
                string[] cells = row.Split(',');
                varModsDataGridView.Rows.Add(cells);
            }
        }

        private void InitializeFromDefaultSettings()
        {
            VarMods = new StringCollection();
            foreach (var item in Settings.Default.VariableMods)
            {
                VarMods.Add(item);
            }
        }

    }
}
