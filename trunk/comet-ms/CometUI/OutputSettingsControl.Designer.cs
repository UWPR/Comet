namespace CometUI
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
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.outputFormatsList = new System.Windows.Forms.CheckedListBox();
            this.numOutputLinesSpinner = new System.Windows.Forms.NumericUpDown();
            ((System.ComponentModel.ISupportInitialize)(this.numOutputLinesSpinner)).BeginInit();
            this.SuspendLayout();
            // 
            // checkBox1
            // 
            this.checkBox1.AutoSize = true;
            this.checkBox1.Location = new System.Drawing.Point(25, 124);
            this.checkBox1.Name = "checkBox1";
            this.checkBox1.Size = new System.Drawing.Size(280, 17);
            this.checkBox1.TabIndex = 32;
            this.checkBox1.Text = "Print expect score in place of SP for SQT and out files";
            this.checkBox1.UseVisualStyleBackColor = true;
            // 
            // outputFormatsList
            // 
            this.outputFormatsList.CheckOnClick = true;
            this.outputFormatsList.FormattingEnabled = true;
            this.outputFormatsList.HorizontalScrollbar = true;
            this.outputFormatsList.Items.AddRange(new object[] {
            "out files",
            "pepXML",
            "pinXML",
            "SQT",
            "text file"});
            this.outputFormatsList.Location = new System.Drawing.Point(25, 29);
            this.outputFormatsList.Name = "outputFormatsList";
            this.outputFormatsList.Size = new System.Drawing.Size(230, 79);
            this.outputFormatsList.TabIndex = 31;
            // 
            // numOutputLinesSpinner
            // 
            this.numOutputLinesSpinner.Location = new System.Drawing.Point(25, 160);
            this.numOutputLinesSpinner.Name = "numOutputLinesSpinner";
            this.numOutputLinesSpinner.Size = new System.Drawing.Size(120, 20);
            this.numOutputLinesSpinner.TabIndex = 33;
            // 
            // OutputSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.numOutputLinesSpinner);
            this.Controls.Add(this.checkBox1);
            this.Controls.Add(this.outputFormatsList);
            this.Name = "OutputSettingsControl";
            this.Size = new System.Drawing.Size(527, 330);
            ((System.ComponentModel.ISupportInitialize)(this.numOutputLinesSpinner)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.CheckBox checkBox1;
        private System.Windows.Forms.CheckedListBox outputFormatsList;
        private System.Windows.Forms.NumericUpDown numOutputLinesSpinner;
    }
}
