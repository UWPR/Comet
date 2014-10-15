using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class ViewResultsPickColumnsControl : UserControl
    {
        private readonly List<Column> _columnsList = new List<Column>();
        public ViewResultsPickColumnsControl()
        {
            InitializeComponent();

            InitializeColumnsList();

            foreach (var item in _columnsList)
            {
                if (item.Show)
                {
                    showColumnsListBox.Items.Add(item.Name);
                }
                else
                {
                    hiddenColumnsListBox.Items.Add(item.Name);
                }
            }
        }

        private void InitializeColumnsList()
        {
            _columnsList.Add(new Column("index", false));
            _columnsList.Add(new Column("assumed_charge", false));
            _columnsList.Add(new Column("precursor_neutral_mass", false));
            _columnsList.Add(new Column("MZratio", false));
            _columnsList.Add(new Column("protein_descr", false));
            _columnsList.Add(new Column("pl", false));
            _columnsList.Add(new Column("retention_time_sec", false));
            _columnsList.Add(new Column("compensation_voltage", false));
            _columnsList.Add(new Column("precursor_intensity", false));
            _columnsList.Add(new Column("collision_energy", false));
            _columnsList.Add(new Column("ppm", false));
            _columnsList.Add(new Column("xcorr", false));
            _columnsList.Add(new Column("deltacn", false));
            _columnsList.Add(new Column("deltacnstar", false));
            _columnsList.Add(new Column("sprank", false));
            _columnsList.Add(new Column("ions", false));
            _columnsList.Add(new Column("num_tol_term", false));
            _columnsList.Add(new Column("num_missed_cleavages", false));
            _columnsList.Add(new Column("massdiff", false));
            _columnsList.Add(new Column("light_area", false));
            _columnsList.Add(new Column("heavy_area", false));
            _columnsList.Add(new Column("fval", false));

            _columnsList.Add(new Column("probability", true));
            _columnsList.Add(new Column("spectrum", true));
            _columnsList.Add(new Column("start_scan", true));
            _columnsList.Add(new Column("spscore", true));
            _columnsList.Add(new Column("ions2", true));
            _columnsList.Add(new Column("peptide", true));
            _columnsList.Add(new Column("protein", true));
            _columnsList.Add(new Column("calc_neutral_pep_mass", true));
            _columnsList.Add(new Column("xpress", true));
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

        class Column
        {
            public String Name { get; set; }
            public bool Show { get; set; }

            public Column(String name, bool show)
            {
                Name = name;
                Show = show;
            }
        }
    }
}
