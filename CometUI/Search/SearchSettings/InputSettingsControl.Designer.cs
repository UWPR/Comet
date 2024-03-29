﻿namespace CometUI.Search.SearchSettings
{
    partial class InputSettingsControl
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
            this.components = new System.ComponentModel.Container();
            this.btnBrowseProteomeDbFile = new System.Windows.Forms.Button();
            this.panelProteinNucleotide = new System.Windows.Forms.Panel();
            this.panelTargetDecoy = new System.Windows.Forms.Panel();
            this.radioButtonTarget = new System.Windows.Forms.RadioButton();
            this.radioButtonDecoyOne = new System.Windows.Forms.RadioButton();
            this.radioButtonDecoyTwo = new System.Windows.Forms.RadioButton();
            this.panelDecoyPrefix = new System.Windows.Forms.Panel();
            this.textBoxDecoyPrefix = new System.Windows.Forms.TextBox();
            this.labelDecoyPrefix = new System.Windows.Forms.Label();
            this.protDbLabel = new System.Windows.Forms.Label();
            this.proteomeDbFileCombo = new System.Windows.Forms.ComboBox();
            this.inputSettingsToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.comboBoxReadingFrame = new System.Windows.Forms.ComboBox();
            this.labelReadingFrame = new System.Windows.Forms.Label();
            this.panelProteinNucleotide.SuspendLayout();
            this.panelTargetDecoy.SuspendLayout();
            this.panelDecoyPrefix.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnBrowseProteomeDbFile
            // 
            this.btnBrowseProteomeDbFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBrowseProteomeDbFile.Location = new System.Drawing.Point(428, 45);
            this.btnBrowseProteomeDbFile.Name = "btnBrowseProteomeDbFile";
            this.btnBrowseProteomeDbFile.Size = new System.Drawing.Size(75, 23);
            this.btnBrowseProteomeDbFile.TabIndex = 36;
            this.btnBrowseProteomeDbFile.Text = "&Browse";
            this.btnBrowseProteomeDbFile.UseVisualStyleBackColor = true;
            this.btnBrowseProteomeDbFile.Click += new System.EventHandler(this.BtnBrowseProteomeDbFileClick);
            // 
            // panelProteinNucleotide
            // 
            this.panelProteinNucleotide.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panelProteinNucleotide.Controls.Add(this.comboBoxReadingFrame);
            this.panelProteinNucleotide.Controls.Add(this.labelReadingFrame);
            this.panelProteinNucleotide.Location = new System.Drawing.Point(21, 141);
            this.panelProteinNucleotide.Name = "panelProteinNucleotide";
            this.panelProteinNucleotide.Size = new System.Drawing.Size(401, 102);
            this.panelProteinNucleotide.TabIndex = 39;
            // 
            // panelTargetDecoy
            // 
            this.panelTargetDecoy.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.panelTargetDecoy.Controls.Add(this.radioButtonTarget);
            this.panelTargetDecoy.Controls.Add(this.radioButtonDecoyOne);
            this.panelTargetDecoy.Controls.Add(this.radioButtonDecoyTwo);
            this.panelTargetDecoy.Controls.Add(this.panelDecoyPrefix);
            this.panelTargetDecoy.Location = new System.Drawing.Point(21, 89);
            this.panelTargetDecoy.Name = "panelTargetDecoy";
            this.panelTargetDecoy.Size = new System.Drawing.Size(401, 52);
            this.panelTargetDecoy.TabIndex = 37;
            // 
            // radioButtonTarget
            // 
            this.radioButtonTarget.AutoSize = true;
            this.radioButtonTarget.Checked = true;
            this.radioButtonTarget.Location = new System.Drawing.Point(4, 3);
            this.radioButtonTarget.Name = "radioButtonTarget";
            this.radioButtonTarget.Size = new System.Drawing.Size(56, 17);
            this.radioButtonTarget.TabIndex = 6;
            this.radioButtonTarget.TabStop = true;
            this.radioButtonTarget.Text = "Target";
            this.inputSettingsToolTip.SetToolTip(this.radioButtonTarget, "No decoy search. ");
            this.radioButtonTarget.UseVisualStyleBackColor = true;
            // 
            // radioButtonDecoyOne
            // 
            this.radioButtonDecoyOne.AutoSize = true;
            this.radioButtonDecoyOne.Location = new System.Drawing.Point(66, 3);
            this.radioButtonDecoyOne.Name = "radioButtonDecoyOne";
            this.radioButtonDecoyOne.Size = new System.Drawing.Size(65, 17);
            this.radioButtonDecoyOne.TabIndex = 7;
            this.radioButtonDecoyOne.Text = "Decoy 1";
            this.inputSettingsToolTip.SetToolTip(this.radioButtonDecoyOne, "Concatenated decoy search.");
            this.radioButtonDecoyOne.UseVisualStyleBackColor = true;
            this.radioButtonDecoyOne.CheckedChanged += new System.EventHandler(this.RadioButtonDecoyOneCheckedChanged);
            // 
            // radioButtonDecoyTwo
            // 
            this.radioButtonDecoyTwo.AutoSize = true;
            this.radioButtonDecoyTwo.Location = new System.Drawing.Point(137, 3);
            this.radioButtonDecoyTwo.Name = "radioButtonDecoyTwo";
            this.radioButtonDecoyTwo.Size = new System.Drawing.Size(65, 17);
            this.radioButtonDecoyTwo.TabIndex = 12;
            this.radioButtonDecoyTwo.TabStop = true;
            this.radioButtonDecoyTwo.Text = "Decoy 2";
            this.inputSettingsToolTip.SetToolTip(this.radioButtonDecoyTwo, "Separate decoy search.");
            this.radioButtonDecoyTwo.UseVisualStyleBackColor = true;
            this.radioButtonDecoyTwo.CheckedChanged += new System.EventHandler(this.RadioButtonDecoyTwoCheckedChanged);
            // 
            // panelDecoyPrefix
            // 
            this.panelDecoyPrefix.Controls.Add(this.textBoxDecoyPrefix);
            this.panelDecoyPrefix.Controls.Add(this.labelDecoyPrefix);
            this.panelDecoyPrefix.Enabled = false;
            this.panelDecoyPrefix.Location = new System.Drawing.Point(77, 22);
            this.panelDecoyPrefix.Name = "panelDecoyPrefix";
            this.panelDecoyPrefix.Size = new System.Drawing.Size(195, 28);
            this.panelDecoyPrefix.TabIndex = 9;
            // 
            // textBoxDecoyPrefix
            // 
            this.textBoxDecoyPrefix.Location = new System.Drawing.Point(76, 3);
            this.textBoxDecoyPrefix.Name = "textBoxDecoyPrefix";
            this.textBoxDecoyPrefix.Size = new System.Drawing.Size(74, 20);
            this.textBoxDecoyPrefix.TabIndex = 10;
            // 
            // labelDecoyPrefix
            // 
            this.labelDecoyPrefix.AutoSize = true;
            this.labelDecoyPrefix.Location = new System.Drawing.Point(4, 6);
            this.labelDecoyPrefix.Name = "labelDecoyPrefix";
            this.labelDecoyPrefix.Size = new System.Drawing.Size(70, 13);
            this.labelDecoyPrefix.TabIndex = 0;
            this.labelDecoyPrefix.Text = "Decoy Prefix:";
            this.inputSettingsToolTip.SetToolTip(this.labelDecoyPrefix, "The prefix string that is pre-pended to the protein identifier and reported for d" +
        "ecoy hits. ");
            // 
            // protDbLabel
            // 
            this.protDbLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.protDbLabel.AutoSize = true;
            this.protDbLabel.Location = new System.Drawing.Point(18, 29);
            this.protDbLabel.Name = "protDbLabel";
            this.protDbLabel.Size = new System.Drawing.Size(139, 13);
            this.protDbLabel.TabIndex = 38;
            this.protDbLabel.Text = "&Proteome Database (.fasta):";
            this.inputSettingsToolTip.SetToolTip(this.protDbLabel, "A full or relative path to the sequence database, in FASTA format, to search.");
            // 
            // proteomeDbFileCombo
            // 
            this.proteomeDbFileCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.proteomeDbFileCombo.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest;
            this.proteomeDbFileCombo.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.AllSystemSources;
            this.proteomeDbFileCombo.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.proteomeDbFileCombo.FormattingEnabled = true;
            this.proteomeDbFileCombo.Location = new System.Drawing.Point(21, 45);
            this.proteomeDbFileCombo.Name = "proteomeDbFileCombo";
            this.proteomeDbFileCombo.Size = new System.Drawing.Size(401, 23);
            this.proteomeDbFileCombo.TabIndex = 35;
            this.proteomeDbFileCombo.SelectedIndexChanged += new System.EventHandler(this.ProteomeDbFileComboSelectedIndexChanged);
            this.proteomeDbFileCombo.TextUpdate += new System.EventHandler(this.ProteomeDbFileComboTextUpdate);
            // 
            // inputSettingsToolTip
            // 
            this.inputSettingsToolTip.AutomaticDelay = 400;
            this.inputSettingsToolTip.IsBalloon = true;
            // 
            // comboBoxReadingFrame
            // 
            this.comboBoxReadingFrame.BackColor = System.Drawing.SystemColors.Window;
            this.comboBoxReadingFrame.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxReadingFrame.FormattingEnabled = true;
            this.comboBoxReadingFrame.Location = new System.Drawing.Point(6, 34);
            this.comboBoxReadingFrame.Name = "comboBoxReadingFrame";
            this.comboBoxReadingFrame.Size = new System.Drawing.Size(223, 21);
            this.comboBoxReadingFrame.TabIndex = 17;
            // 
            // labelReadingFrame
            // 
            this.labelReadingFrame.AutoSize = true;
            this.labelReadingFrame.Location = new System.Drawing.Point(3, 18);
            this.labelReadingFrame.Name = "labelReadingFrame";
            this.labelReadingFrame.Size = new System.Drawing.Size(83, 13);
            this.labelReadingFrame.TabIndex = 16;
            this.labelReadingFrame.Text = "Database Type:";
            this.inputSettingsToolTip.SetToolTip(this.labelReadingFrame, "Specify which sets of reading frames are translated.");
            // 
            // InputSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.btnBrowseProteomeDbFile);
            this.Controls.Add(this.panelProteinNucleotide);
            this.Controls.Add(this.proteomeDbFileCombo);
            this.Controls.Add(this.panelTargetDecoy);
            this.Controls.Add(this.protDbLabel);
            this.Name = "InputSettingsControl";
            this.Size = new System.Drawing.Size(527, 450);
            this.panelProteinNucleotide.ResumeLayout(false);
            this.panelProteinNucleotide.PerformLayout();
            this.panelTargetDecoy.ResumeLayout(false);
            this.panelTargetDecoy.PerformLayout();
            this.panelDecoyPrefix.ResumeLayout(false);
            this.panelDecoyPrefix.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnBrowseProteomeDbFile;
        private System.Windows.Forms.Panel panelProteinNucleotide;
        private System.Windows.Forms.Panel panelTargetDecoy;
        private System.Windows.Forms.RadioButton radioButtonTarget;
        private System.Windows.Forms.RadioButton radioButtonDecoyOne;
        private System.Windows.Forms.RadioButton radioButtonDecoyTwo;
        private System.Windows.Forms.Panel panelDecoyPrefix;
        private System.Windows.Forms.TextBox textBoxDecoyPrefix;
        private System.Windows.Forms.Label labelDecoyPrefix;
        private System.Windows.Forms.Label protDbLabel;
        private System.Windows.Forms.ComboBox proteomeDbFileCombo;
        private System.Windows.Forms.ToolTip inputSettingsToolTip;
        private System.Windows.Forms.ComboBox comboBoxReadingFrame;
        private System.Windows.Forms.Label labelReadingFrame;
    }
}
