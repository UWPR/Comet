using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Windows.Forms;
using System.IO;
using CometUI.Properties;

namespace CometUI
{
    public partial class EnzymeSettingsControl : UserControl
    {
        public List<String[]> EnzymeInfo { get; set; } 
        private int SearchEnzymeComboEditListIndex { get; set; }
        private int SampleEnzymeComboEditListIndex { get; set; }

        private new Form  Parent { get; set; }
        private readonly Dictionary<string, int> _enzymeTermini = new Dictionary<string, int>();

        public EnzymeSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            _enzymeTermini.Add("Fully-digested", 2);
            _enzymeTermini.Add("Semi-digested", 1);
            _enzymeTermini.Add("N-term", 8);
            _enzymeTermini.Add("C-term", 9);

            Dictionary<string, int>.KeyCollection enzymeTerminiKeys = _enzymeTermini.Keys;
            foreach (var key in enzymeTerminiKeys)
            {
                enzymeTerminiCombo.Items.Add(key);
            }

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            int enzymeTerminiDefaultIndex = enzymeTerminiCombo.FindString(Settings.Default.EnzymeTermini);
            enzymeTerminiCombo.SelectedIndex = enzymeTerminiDefaultIndex;

            // For this particular combo, index == value of allowed missed cleavages
            missedCleavagesCombo.SelectedIndex = Settings.Default.AllowedMissedCleavages;            
        
            EnzymeInfo = new List<string[]>();
            foreach (var item in Settings.Default.EnzymeInfo)
            {
                string[] row = item.Split(',');
                EnzymeInfo.Add(row);
                sampleEnzymeCombo.Items.Add(row[1]);
                searchEnzymeCombo.Items.Add(row[1]);
            }

            // Add the "Edit List" item at the end of the lists
            searchEnzymeCombo.Items.Add("<Edit List...>");
            SearchEnzymeComboEditListIndex = searchEnzymeCombo.Items.Count - 1;
            sampleEnzymeCombo.Items.Add("<Edit List...>");
            SampleEnzymeComboEditListIndex = sampleEnzymeCombo.Items.Count - 1;

            sampleEnzymeCombo.SelectedIndex = Settings.Default.SampleEnzymeNumber;
            searchEnzymeCombo.SelectedIndex = Settings.Default.SearchEnzymeNumber;
        }

        private void SearchEnzymeComboSelectedIndexChanged(object sender, EventArgs e)
        {
            // Todo: Ask Jimmy what to do once the user OK's or Cancel's out of the EnzymeInfoDlg
            // Do we set the combobox selection back to what it was?  Do we require users select 
            // an item from the data grid list?
            var srchEnzymeCombo = (ComboBox) sender;
            if (SearchEnzymeComboEditListIndex == srchEnzymeCombo.SelectedIndex)
            {
                var dlgEnzymeInfo = new EnzymeInfoDlg(this);
                if (DialogResult.OK == dlgEnzymeInfo.ShowDialog())
                {

                }
            }
        }

        private void SampleEnzymeComboSelectedIndexChanged(object sender, EventArgs e)
        {
            var smplEnzymeCombo = (ComboBox)sender;
            if (SampleEnzymeComboEditListIndex == smplEnzymeCombo.SelectedIndex)
            {
                var dlgEnzymeInfo = new EnzymeInfoDlg(this);
                if (DialogResult.OK == dlgEnzymeInfo.ShowDialog())
                {

                }
            }
        }
    }
}
