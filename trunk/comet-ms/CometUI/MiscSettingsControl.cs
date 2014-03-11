using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace CometUI
{
    public partial class MiscSettingsControl : UserControl
    {
        private new Form Parent { get; set; }

        private readonly Dictionary<string, int> _nucleotideReadingFrame = new Dictionary<string, int>();

        public MiscSettingsControl(Form parent)
        {
            InitializeComponent();

            Parent = parent;

            _nucleotideReadingFrame.Add("Protein Sequence DB", 0);
            _nucleotideReadingFrame.Add("1st Forward", 1);
            _nucleotideReadingFrame.Add("2nd Forward", 2);
            _nucleotideReadingFrame.Add("3rd Forward", 3);
            _nucleotideReadingFrame.Add("1st Reverse", 4);
            _nucleotideReadingFrame.Add("2nd Reverse", 5);
            _nucleotideReadingFrame.Add("3rd Reverse", 6);
            _nucleotideReadingFrame.Add("All Three Forward", 7);
            _nucleotideReadingFrame.Add("All Three Reverse", 8);
            _nucleotideReadingFrame.Add("All Six Reading Frames", 9);

            Dictionary<string, int>.KeyCollection nucleotideReadingFrameKeys = _nucleotideReadingFrame.Keys;
            foreach (var key in nucleotideReadingFrameKeys)
            {
                nucleotideReadingFrameCombo.Items.Add(key);
            }

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
        }
    }
}
