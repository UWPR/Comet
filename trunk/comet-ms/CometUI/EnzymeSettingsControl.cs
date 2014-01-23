using System;
using System.Collections.Generic;
using System.Windows.Forms;
using CometUI.Properties;
using System.Collections.Specialized;

namespace CometUI
{
    public partial class EnzymeSettingsControl : UserControl
    {
        public StringCollection EnzymeInfo { get; set; } 

        private int SearchEnzymeComboEditListIndex { get; set; }
        private int SampleEnzymeComboEditListIndex { get; set; }
        private int SearchEnzymeCurrentSelectedIndex { get; set; }
        private int SampleEnzymeCurrentSelectedIndex { get; set; }

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


            EnzymeInfo = new StringCollection();
            foreach (var item in Settings.Default.EnzymeInfo)
            {
                EnzymeInfo.Add(item);
            }

            UpdateEnzymeInfo();

            SearchEnzymeCurrentSelectedIndex = Settings.Default.SearchEnzymeNumber;
            SampleEnzymeCurrentSelectedIndex = Settings.Default.SampleEnzymeNumber;

            searchEnzymeCombo.SelectedIndex = SearchEnzymeCurrentSelectedIndex;
            sampleEnzymeCombo.SelectedIndex = SampleEnzymeCurrentSelectedIndex;
        }

        private void UpdateEnzymeInfo()
        {
            sampleEnzymeCombo.Items.Clear();
            searchEnzymeCombo.Items.Clear();

            foreach (var row in EnzymeInfo)
            {
                sampleEnzymeCombo.Items.Add(row);
                searchEnzymeCombo.Items.Add(row);
            }

            // Add the "Edit List" item at the end of the lists
            searchEnzymeCombo.Items.Add("<Edit List...>");
            SearchEnzymeComboEditListIndex = searchEnzymeCombo.Items.Count - 1;
            sampleEnzymeCombo.Items.Add("<Edit List...>");
            SampleEnzymeComboEditListIndex = sampleEnzymeCombo.Items.Count - 1;
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
                    if (dlgEnzymeInfo.EnzymeInfoChanged)
                    {
                        UpdateEnzymeInfo();
                    }
                }
                else
                {
                    srchEnzymeCombo.SelectedIndex = SearchEnzymeCurrentSelectedIndex;
                }
            }
            else
            {
                SearchEnzymeCurrentSelectedIndex = srchEnzymeCombo.SelectedIndex;
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
                    if (dlgEnzymeInfo.EnzymeInfoChanged)
                    {
                        UpdateEnzymeInfo();
                    }
                }
                else
                {
                    smplEnzymeCombo.SelectedIndex = SampleEnzymeCurrentSelectedIndex;
                }
            }
            else
            {
                SampleEnzymeCurrentSelectedIndex = smplEnzymeCombo.SelectedIndex;
            }
        }
    }
}
