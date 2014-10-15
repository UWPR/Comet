using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class ViewResultsPickColumnsControl : UserControl
    {
        public ViewResultsPickColumnsControl()
        {
            InitializeComponent();
        }

        private void HiddenColumnsListBoxKeyUp(object sender, KeyEventArgs e)
        {
            SelectAllListBoxItems(hiddenColumnsListBox, e);
        }

        private void ShowColumnsListBoxKeyUp(object sender, KeyEventArgs e)
        {
            SelectAllListBoxItems(showColumnsListBox, e);
        }

        private static void SelectAllListBoxItems(ListBox list, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.A && e.Control)
            {
                for (int i = 0; i < list.Items.Count; i++)
                {
                    list.SetSelected(i, true);
                }
            }
        }
    }
}
