using System;
using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class ViewResultsOtherActionsControl : UserControl
    {
        private ViewSearchResultsControl ViewSearchResultsControl { get; set; }

        public ViewResultsOtherActionsControl(ViewSearchResultsControl parent)
        {
            InitializeComponent();

            ViewSearchResultsControl = parent;
        }

        private void ExportResultsBtnClick(object sender, EventArgs e)
        {
            ViewSearchResultsControl.ExportResultsList();
        }
    }
}
