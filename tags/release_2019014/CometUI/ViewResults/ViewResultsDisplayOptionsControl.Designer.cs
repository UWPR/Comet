namespace CometUI.ViewResults
{
    partial class ViewResultsDisplayOptionsControl
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
            this.displayOptionsMainPanel = new System.Windows.Forms.Panel();
            this.btnUpdateResults = new System.Windows.Forms.Button();
            this.columnHeadersRadioButtonsPanel = new System.Windows.Forms.Panel();
            this.columnHeadersRegularRadioButton = new System.Windows.Forms.RadioButton();
            this.columnHeadersCondensedRadioButton = new System.Windows.Forms.RadioButton();
            this.columnHeadersLabel = new System.Windows.Forms.Label();
            this.displayOptionsMainPanel.SuspendLayout();
            this.columnHeadersRadioButtonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // displayOptionsMainPanel
            // 
            this.displayOptionsMainPanel.Controls.Add(this.btnUpdateResults);
            this.displayOptionsMainPanel.Controls.Add(this.columnHeadersRadioButtonsPanel);
            this.displayOptionsMainPanel.Controls.Add(this.columnHeadersLabel);
            this.displayOptionsMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.displayOptionsMainPanel.Location = new System.Drawing.Point(0, 0);
            this.displayOptionsMainPanel.Name = "displayOptionsMainPanel";
            this.displayOptionsMainPanel.Size = new System.Drawing.Size(1000, 210);
            this.displayOptionsMainPanel.TabIndex = 0;
            // 
            // btnUpdateResults
            // 
            this.btnUpdateResults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnUpdateResults.Location = new System.Drawing.Point(908, 163);
            this.btnUpdateResults.Name = "btnUpdateResults";
            this.btnUpdateResults.Size = new System.Drawing.Size(75, 23);
            this.btnUpdateResults.TabIndex = 59;
            this.btnUpdateResults.Text = "&Update";
            this.btnUpdateResults.UseVisualStyleBackColor = true;
            this.btnUpdateResults.Click += new System.EventHandler(this.BtnUpdateResultsClick);
            // 
            // columnHeadersRadioButtonsPanel
            // 
            this.columnHeadersRadioButtonsPanel.Controls.Add(this.columnHeadersRegularRadioButton);
            this.columnHeadersRadioButtonsPanel.Controls.Add(this.columnHeadersCondensedRadioButton);
            this.columnHeadersRadioButtonsPanel.Location = new System.Drawing.Point(20, 54);
            this.columnHeadersRadioButtonsPanel.Name = "columnHeadersRadioButtonsPanel";
            this.columnHeadersRadioButtonsPanel.Size = new System.Drawing.Size(244, 21);
            this.columnHeadersRadioButtonsPanel.TabIndex = 58;
            // 
            // columnHeadersRegularRadioButton
            // 
            this.columnHeadersRegularRadioButton.AutoSize = true;
            this.columnHeadersRegularRadioButton.Location = new System.Drawing.Point(3, 3);
            this.columnHeadersRegularRadioButton.Name = "columnHeadersRegularRadioButton";
            this.columnHeadersRegularRadioButton.Size = new System.Drawing.Size(62, 17);
            this.columnHeadersRegularRadioButton.TabIndex = 52;
            this.columnHeadersRegularRadioButton.TabStop = true;
            this.columnHeadersRegularRadioButton.Text = "Regular";
            this.columnHeadersRegularRadioButton.UseVisualStyleBackColor = true;
            this.columnHeadersRegularRadioButton.CheckedChanged += new System.EventHandler(this.ColumnHeadersRegularRadioButtonCheckedChanged);
            // 
            // columnHeadersCondensedRadioButton
            // 
            this.columnHeadersCondensedRadioButton.AutoSize = true;
            this.columnHeadersCondensedRadioButton.Location = new System.Drawing.Point(89, 4);
            this.columnHeadersCondensedRadioButton.Name = "columnHeadersCondensedRadioButton";
            this.columnHeadersCondensedRadioButton.Size = new System.Drawing.Size(79, 17);
            this.columnHeadersCondensedRadioButton.TabIndex = 53;
            this.columnHeadersCondensedRadioButton.TabStop = true;
            this.columnHeadersCondensedRadioButton.Text = "Condensed";
            this.columnHeadersCondensedRadioButton.UseVisualStyleBackColor = true;
            this.columnHeadersCondensedRadioButton.CheckedChanged += new System.EventHandler(this.ColumnHeadersCondensedRadioButtonCheckedChanged);
            // 
            // columnHeadersLabel
            // 
            this.columnHeadersLabel.AutoSize = true;
            this.columnHeadersLabel.Location = new System.Drawing.Point(18, 34);
            this.columnHeadersLabel.Name = "columnHeadersLabel";
            this.columnHeadersLabel.Size = new System.Drawing.Size(86, 13);
            this.columnHeadersLabel.TabIndex = 51;
            this.columnHeadersLabel.Text = "Column headers:";
            // 
            // ViewResultsDisplayOptionsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.displayOptionsMainPanel);
            this.Name = "ViewResultsDisplayOptionsControl";
            this.Size = new System.Drawing.Size(1000, 210);
            this.displayOptionsMainPanel.ResumeLayout(false);
            this.displayOptionsMainPanel.PerformLayout();
            this.columnHeadersRadioButtonsPanel.ResumeLayout(false);
            this.columnHeadersRadioButtonsPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel displayOptionsMainPanel;
        private System.Windows.Forms.Label columnHeadersLabel;
        private System.Windows.Forms.RadioButton columnHeadersCondensedRadioButton;
        private System.Windows.Forms.RadioButton columnHeadersRegularRadioButton;
        private System.Windows.Forms.Panel columnHeadersRadioButtonsPanel;
        private System.Windows.Forms.Button btnUpdateResults;

    }
}
