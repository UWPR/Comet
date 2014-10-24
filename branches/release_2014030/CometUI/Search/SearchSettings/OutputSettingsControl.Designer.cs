namespace CometUI.Search.SearchSettings
{
    partial class OutputSettingsControl
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
            this.sqtExpectScoreCheckBox = new System.Windows.Forms.CheckBox();
            this.numOutputLinesSpinner = new System.Windows.Forms.NumericUpDown();
            this.outFileCheckBox = new System.Windows.Forms.CheckBox();
            this.percolatorCheckBox = new System.Windows.Forms.CheckBox();
            this.pepXMLCheckBox = new System.Windows.Forms.CheckBox();
            this.sqtCheckBox = new System.Windows.Forms.CheckBox();
            this.textCheckBox = new System.Windows.Forms.CheckBox();
            this.outExpectScoreCheckBox = new System.Windows.Forms.CheckBox();
            this.outShowFragmentIonsCheckBox = new System.Windows.Forms.CheckBox();
            this.label1 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.sqtToStdoutCheckBox = new System.Windows.Forms.CheckBox();
            this.outSkipReSearchingCheckBox = new System.Windows.Forms.CheckBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.outputSettingsToolTip = new System.Windows.Forms.ToolTip(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.numOutputLinesSpinner)).BeginInit();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // sqtExpectScoreCheckBox
            // 
            this.sqtExpectScoreCheckBox.AutoSize = true;
            this.sqtExpectScoreCheckBox.Enabled = false;
            this.sqtExpectScoreCheckBox.Location = new System.Drawing.Point(38, 145);
            this.sqtExpectScoreCheckBox.Name = "sqtExpectScoreCheckBox";
            this.sqtExpectScoreCheckBox.Size = new System.Drawing.Size(180, 17);
            this.sqtExpectScoreCheckBox.TabIndex = 5;
            this.sqtExpectScoreCheckBox.Text = "Print expect score in place of SP";
            this.sqtExpectScoreCheckBox.UseVisualStyleBackColor = true;
            // 
            // numOutputLinesSpinner
            // 
            this.numOutputLinesSpinner.Location = new System.Drawing.Point(112, 32);
            this.numOutputLinesSpinner.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.numOutputLinesSpinner.Name = "numOutputLinesSpinner";
            this.numOutputLinesSpinner.Size = new System.Drawing.Size(74, 20);
            this.numOutputLinesSpinner.TabIndex = 10;
            this.numOutputLinesSpinner.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            // 
            // outFileCheckBox
            // 
            this.outFileCheckBox.AutoSize = true;
            this.outFileCheckBox.Location = new System.Drawing.Point(21, 168);
            this.outFileCheckBox.Name = "outFileCheckBox";
            this.outFileCheckBox.Size = new System.Drawing.Size(109, 17);
            this.outFileCheckBox.TabIndex = 6;
            this.outFileCheckBox.Text = "Generate out files";
            this.outFileCheckBox.UseVisualStyleBackColor = true;
            this.outFileCheckBox.CheckedChanged += new System.EventHandler(this.OutFileCheckBoxCheckedChanged);
            // 
            // percolatorCheckBox
            // 
            this.percolatorCheckBox.AutoSize = true;
            this.percolatorCheckBox.Location = new System.Drawing.Point(21, 57);
            this.percolatorCheckBox.Name = "percolatorCheckBox";
            this.percolatorCheckBox.Size = new System.Drawing.Size(136, 17);
            this.percolatorCheckBox.TabIndex = 2;
            this.percolatorCheckBox.Text = "Generate percolator file";
            this.percolatorCheckBox.UseVisualStyleBackColor = true;
            // 
            // pepXMLCheckBox
            // 
            this.pepXMLCheckBox.AutoSize = true;
            this.pepXMLCheckBox.Location = new System.Drawing.Point(21, 34);
            this.pepXMLCheckBox.Name = "pepXMLCheckBox";
            this.pepXMLCheckBox.Size = new System.Drawing.Size(129, 17);
            this.pepXMLCheckBox.TabIndex = 1;
            this.pepXMLCheckBox.Text = "Generate pepXML file";
            this.pepXMLCheckBox.UseVisualStyleBackColor = true;
            // 
            // sqtCheckBox
            // 
            this.sqtCheckBox.AutoSize = true;
            this.sqtCheckBox.Location = new System.Drawing.Point(21, 126);
            this.sqtCheckBox.Name = "sqtCheckBox";
            this.sqtCheckBox.Size = new System.Drawing.Size(111, 17);
            this.sqtCheckBox.TabIndex = 4;
            this.sqtCheckBox.Text = "Generate SQT file";
            this.sqtCheckBox.UseVisualStyleBackColor = true;
            this.sqtCheckBox.CheckedChanged += new System.EventHandler(this.SqtCheckBoxCheckedChanged);
            // 
            // textCheckBox
            // 
            this.textCheckBox.AutoSize = true;
            this.textCheckBox.Location = new System.Drawing.Point(21, 80);
            this.textCheckBox.Name = "textCheckBox";
            this.textCheckBox.Size = new System.Drawing.Size(106, 17);
            this.textCheckBox.TabIndex = 3;
            this.textCheckBox.Text = "Generate text file";
            this.textCheckBox.UseVisualStyleBackColor = true;
            // 
            // outExpectScoreCheckBox
            // 
            this.outExpectScoreCheckBox.AutoSize = true;
            this.outExpectScoreCheckBox.Enabled = false;
            this.outExpectScoreCheckBox.Location = new System.Drawing.Point(38, 187);
            this.outExpectScoreCheckBox.Name = "outExpectScoreCheckBox";
            this.outExpectScoreCheckBox.Size = new System.Drawing.Size(180, 17);
            this.outExpectScoreCheckBox.TabIndex = 7;
            this.outExpectScoreCheckBox.Text = "Print expect score in place of SP";
            this.outExpectScoreCheckBox.UseVisualStyleBackColor = true;
            // 
            // outShowFragmentIonsCheckBox
            // 
            this.outShowFragmentIonsCheckBox.AutoSize = true;
            this.outShowFragmentIonsCheckBox.Enabled = false;
            this.outShowFragmentIonsCheckBox.Location = new System.Drawing.Point(38, 206);
            this.outShowFragmentIonsCheckBox.Name = "outShowFragmentIonsCheckBox";
            this.outShowFragmentIonsCheckBox.Size = new System.Drawing.Size(119, 17);
            this.outShowFragmentIonsCheckBox.TabIndex = 8;
            this.outShowFragmentIonsCheckBox.Text = "Show fragment ions";
            this.outShowFragmentIonsCheckBox.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(17, 34);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(89, 13);
            this.label1.TabIndex = 41;
            this.label1.Text = "Num output lines:";
            this.outputSettingsToolTip.SetToolTip(this.label1, "The number of search result hits (peptides) that are reported for each spectrum q" +
        "uery.");
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.sqtToStdoutCheckBox);
            this.groupBox1.Controls.Add(this.outSkipReSearchingCheckBox);
            this.groupBox1.Controls.Add(this.pepXMLCheckBox);
            this.groupBox1.Controls.Add(this.percolatorCheckBox);
            this.groupBox1.Controls.Add(this.outShowFragmentIonsCheckBox);
            this.groupBox1.Controls.Add(this.textCheckBox);
            this.groupBox1.Controls.Add(this.outExpectScoreCheckBox);
            this.groupBox1.Controls.Add(this.sqtCheckBox);
            this.groupBox1.Controls.Add(this.outFileCheckBox);
            this.groupBox1.Controls.Add(this.sqtExpectScoreCheckBox);
            this.groupBox1.Location = new System.Drawing.Point(22, 21);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(225, 267);
            this.groupBox1.TabIndex = 42;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Output Formats";
            // 
            // sqtToStdoutCheckBox
            // 
            this.sqtToStdoutCheckBox.AutoSize = true;
            this.sqtToStdoutCheckBox.Location = new System.Drawing.Point(21, 103);
            this.sqtToStdoutCheckBox.Name = "sqtToStdoutCheckBox";
            this.sqtToStdoutCheckBox.Size = new System.Drawing.Size(165, 17);
            this.sqtToStdoutCheckBox.TabIndex = 10;
            this.sqtToStdoutCheckBox.Text = "Write SQT to standard output";
            this.sqtToStdoutCheckBox.UseVisualStyleBackColor = true;
            // 
            // outSkipReSearchingCheckBox
            // 
            this.outSkipReSearchingCheckBox.AutoSize = true;
            this.outSkipReSearchingCheckBox.Enabled = false;
            this.outSkipReSearchingCheckBox.Location = new System.Drawing.Point(38, 225);
            this.outSkipReSearchingCheckBox.Name = "outSkipReSearchingCheckBox";
            this.outSkipReSearchingCheckBox.Size = new System.Drawing.Size(108, 17);
            this.outSkipReSearchingCheckBox.TabIndex = 9;
            this.outSkipReSearchingCheckBox.Text = "Skip re-searching";
            this.outSkipReSearchingCheckBox.UseVisualStyleBackColor = true;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.numOutputLinesSpinner);
            this.groupBox2.Controls.Add(this.label1);
            this.groupBox2.Location = new System.Drawing.Point(274, 21);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(225, 267);
            this.groupBox2.TabIndex = 43;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Options";
            // 
            // OutputSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Name = "OutputSettingsControl";
            this.Size = new System.Drawing.Size(527, 330);
            ((System.ComponentModel.ISupportInitialize)(this.numOutputLinesSpinner)).EndInit();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.CheckBox sqtExpectScoreCheckBox;
        private System.Windows.Forms.NumericUpDown numOutputLinesSpinner;
        private System.Windows.Forms.CheckBox outFileCheckBox;
        private System.Windows.Forms.CheckBox percolatorCheckBox;
        private System.Windows.Forms.CheckBox pepXMLCheckBox;
        private System.Windows.Forms.CheckBox sqtCheckBox;
        private System.Windows.Forms.CheckBox textCheckBox;
        private System.Windows.Forms.CheckBox outExpectScoreCheckBox;
        private System.Windows.Forms.CheckBox outShowFragmentIonsCheckBox;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.CheckBox outSkipReSearchingCheckBox;
        private System.Windows.Forms.CheckBox sqtToStdoutCheckBox;
        private System.Windows.Forms.ToolTip outputSettingsToolTip;
    }
}
