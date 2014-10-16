using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class ViewResultsPickColumnsControl : UserControl
    {
        public ViewResultsPickColumnsControl()
        {
            InitializeComponent();
        }

        private void MoveUp(ListBox listBox)
        {
            int selectedIndex = listBox.SelectedIndex;
            if (selectedIndex > 0 & selectedIndex != -1)
            {
                listBox.Items.Insert(selectedIndex - 1, listBox.Items[selectedIndex]);
                listBox.Items.RemoveAt(selectedIndex + 1);
                listBox.SelectedIndex = selectedIndex - 1;
            }
        }

        private void MoveDown(ListBox listBox)
        {
            int selectedIndex = listBox.SelectedIndex;
            if (selectedIndex < listBox.Items.Count - 1 & selectedIndex != -1)
            {
                listBox.Items.Insert(selectedIndex + 2, listBox.Items[selectedIndex]);
                listBox.Items.RemoveAt(selectedIndex);
                listBox.SelectedIndex = selectedIndex + 1;

            }
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

        private void BtnMoveUpClick(object sender, System.EventArgs e)
        {
            MoveUp(showColumnsListBox);
        }

        private void BtnMoveDownClick(object sender, System.EventArgs e)
        {
            MoveDown(showColumnsListBox);
        }
    }
}
