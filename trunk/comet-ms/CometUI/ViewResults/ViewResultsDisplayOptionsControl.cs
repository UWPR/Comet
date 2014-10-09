using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class ViewResultsDisplayOptionsControl : UserControl
    {
        public ViewResultsDisplayOptionsControl()
        {
            InitializeComponent();

            rowsPerPageCombo.SelectedIndex = 2;
        }
    }
}
