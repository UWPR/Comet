namespace CometUI.ViewResults
{
    partial class ViewResultsPickColumnsControl
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.pickColumnsMainPanel = new System.Windows.Forms.Panel();
            this.button4 = new System.Windows.Forms.Button();
            this.button3 = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.hiddenColumnsLabel = new System.Windows.Forms.Label();
            this.btnUpdateResults = new System.Windows.Forms.Button();
            this.showColumnsListBox = new System.Windows.Forms.ListBox();
            this.hiddenColumnsListBox = new System.Windows.Forms.ListBox();
            this.pickColumnsMainPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // pickColumnsMainPanel
            // 
            this.pickColumnsMainPanel.AutoScroll = true;
            this.pickColumnsMainPanel.Controls.Add(this.button4);
            this.pickColumnsMainPanel.Controls.Add(this.button3);
            this.pickColumnsMainPanel.Controls.Add(this.button2);
            this.pickColumnsMainPanel.Controls.Add(this.button1);
            this.pickColumnsMainPanel.Controls.Add(this.label1);
            this.pickColumnsMainPanel.Controls.Add(this.hiddenColumnsLabel);
            this.pickColumnsMainPanel.Controls.Add(this.btnUpdateResults);
            this.pickColumnsMainPanel.Controls.Add(this.showColumnsListBox);
            this.pickColumnsMainPanel.Controls.Add(this.hiddenColumnsListBox);
            this.pickColumnsMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pickColumnsMainPanel.Location = new System.Drawing.Point(0, 0);
            this.pickColumnsMainPanel.Name = "pickColumnsMainPanel";
            this.pickColumnsMainPanel.Size = new System.Drawing.Size(653, 220);
            this.pickColumnsMainPanel.TabIndex = 0;
            // 
            // button4
            // 
            this.button4.Location = new System.Drawing.Point(561, 84);
            this.button4.Name = "button4";
            this.button4.Size = new System.Drawing.Size(75, 23);
            this.button4.TabIndex = 51;
            this.button4.Text = "Down";
            this.button4.UseVisualStyleBackColor = true;
            // 
            // button3
            // 
            this.button3.Location = new System.Drawing.Point(561, 45);
            this.button3.Name = "button3";
            this.button3.Size = new System.Drawing.Size(75, 23);
            this.button3.TabIndex = 50;
            this.button3.Text = "Up";
            this.button3.UseVisualStyleBackColor = true;
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(247, 84);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(75, 23);
            this.button2.TabIndex = 49;
            this.button2.Text = ">>";
            this.button2.UseVisualStyleBackColor = true;
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(247, 45);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 23);
            this.button1.TabIndex = 48;
            this.button1.Text = "<<";
            this.button1.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(336, 20);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(80, 13);
            this.label1.TabIndex = 47;
            this.label1.Text = "Show Columns:";
            // 
            // hiddenColumnsLabel
            // 
            this.hiddenColumnsLabel.AutoSize = true;
            this.hiddenColumnsLabel.Location = new System.Drawing.Point(23, 20);
            this.hiddenColumnsLabel.Name = "hiddenColumnsLabel";
            this.hiddenColumnsLabel.Size = new System.Drawing.Size(75, 13);
            this.hiddenColumnsLabel.TabIndex = 46;
            this.hiddenColumnsLabel.Text = "Hide Columns:";
            // 
            // btnUpdateResults
            // 
            this.btnUpdateResults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnUpdateResults.Location = new System.Drawing.Point(561, 180);
            this.btnUpdateResults.Name = "btnUpdateResults";
            this.btnUpdateResults.Size = new System.Drawing.Size(75, 23);
            this.btnUpdateResults.TabIndex = 45;
            this.btnUpdateResults.Text = "&Update";
            this.btnUpdateResults.UseVisualStyleBackColor = true;
            // 
            // showColumnsListBox
            // 
            this.showColumnsListBox.FormattingEnabled = true;
            this.showColumnsListBox.Items.AddRange(new object[] {
            "probability",
            "spectrum",
            "start_scan",
            "spscore",
            "ions2",
            "peptide",
            "protein",
            "calc_neutral_pep_mass",
            "xpress"});
            this.showColumnsListBox.Location = new System.Drawing.Point(339, 45);
            this.showColumnsListBox.Name = "showColumnsListBox";
            this.showColumnsListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
            this.showColumnsListBox.Size = new System.Drawing.Size(205, 121);
            this.showColumnsListBox.TabIndex = 1;
            // 
            // hiddenColumnsListBox
            // 
            this.hiddenColumnsListBox.FormattingEnabled = true;
            this.hiddenColumnsListBox.Items.AddRange(new object[] {
            "index",
            "assumed_charge",
            "precursor_neutral_mass",
            "MZratio",
            "protein_descr",
            "pl",
            "retention_time_sec",
            "compensation_voltage",
            "precursor_intensity",
            "collision_energy",
            "ppm",
            "xcorr",
            "deltacn",
            "deltacnstar",
            "sprank",
            "ions",
            "num_tol_term",
            "num_missed_cleavages",
            "massdiff",
            "light_area",
            "heavy_area",
            "fval"});
            this.hiddenColumnsListBox.Location = new System.Drawing.Point(26, 45);
            this.hiddenColumnsListBox.Name = "hiddenColumnsListBox";
            this.hiddenColumnsListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
            this.hiddenColumnsListBox.Size = new System.Drawing.Size(205, 121);
            this.hiddenColumnsListBox.TabIndex = 0;
            // 
            // ViewResultsPickColumnsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.pickColumnsMainPanel);
            this.Name = "ViewResultsPickColumnsControl";
            this.Size = new System.Drawing.Size(653, 220);
            this.pickColumnsMainPanel.ResumeLayout(false);
            this.pickColumnsMainPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel pickColumnsMainPanel;
        private System.Windows.Forms.ListBox showColumnsListBox;
        private System.Windows.Forms.ListBox hiddenColumnsListBox;
        private System.Windows.Forms.Button btnUpdateResults;
        private System.Windows.Forms.Label hiddenColumnsLabel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button button4;
        private System.Windows.Forms.Button button3;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Button button1;
    }
}
