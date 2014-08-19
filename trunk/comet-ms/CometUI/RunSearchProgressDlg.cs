using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using CometUI.CustomControls;

namespace CometUI
{
    public partial class RunSearchProgressDlg : ProgressDlg
    {
        private CometSearch CometSearch { get; set; }

        public RunSearchProgressDlg(CometSearch cometSearch, BackgroundWorker backgroundWorker)
            : base(backgroundWorker)
        {
            InitializeComponent();

            CometSearch = cometSearch;
        }

        public override void Cancel()
        {
            CometSearch.CancelSearch();
            base.Cancel();
        }
    }
}
