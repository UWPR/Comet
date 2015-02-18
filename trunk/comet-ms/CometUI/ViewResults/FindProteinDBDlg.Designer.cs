namespace CometUI.ViewResults
{
    partial class FindProteinDBDlg
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FindProteinDBDlg));
            this.btnOK = new System.Windows.Forms.Button();
            this.btnBrowseSearchDBFile = new System.Windows.Forms.Button();
            this.searchDbLabel = new System.Windows.Forms.Label();
            this.btnCancel = new System.Windows.Forms.Button();
            this.searchDBFileCombo = new System.Windows.Forms.ComboBox();
            this.SuspendLayout();
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOK.Enabled = false;
            this.btnOK.Location = new System.Drawing.Point(400, 103);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 107;
            this.btnOK.Text = "&OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.BtnOKClick);
            // 
            // btnBrowseSearchDBFile
            // 
            this.btnBrowseSearchDBFile.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBrowseSearchDBFile.Location = new System.Drawing.Point(481, 36);
            this.btnBrowseSearchDBFile.Name = "btnBrowseSearchDBFile";
            this.btnBrowseSearchDBFile.Size = new System.Drawing.Size(75, 23);
            this.btnBrowseSearchDBFile.TabIndex = 106;
            this.btnBrowseSearchDBFile.Text = "&Browse";
            this.btnBrowseSearchDBFile.UseVisualStyleBackColor = true;
            this.btnBrowseSearchDBFile.Click += new System.EventHandler(this.BtnBrowseSearchDBFileClick);
            // 
            // searchDbLabel
            // 
            this.searchDbLabel.AutoSize = true;
            this.searchDbLabel.Location = new System.Drawing.Point(12, 20);
            this.searchDbLabel.Name = "searchDbLabel";
            this.searchDbLabel.Size = new System.Drawing.Size(112, 13);
            this.searchDbLabel.TabIndex = 104;
            this.searchDbLabel.Text = "&Search Database File:";
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.Location = new System.Drawing.Point(481, 103);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 108;
            this.btnCancel.Text = "&Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.BtnCancelClick);
            // 
            // searchDBFileCombo
            // 
            this.searchDBFileCombo.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest;
            this.searchDBFileCombo.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.AllSystemSources;
            this.searchDBFileCombo.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F);
            this.searchDBFileCombo.FormattingEnabled = true;
            this.searchDBFileCombo.Location = new System.Drawing.Point(15, 36);
            this.searchDBFileCombo.Name = "searchDBFileCombo";
            this.searchDBFileCombo.Size = new System.Drawing.Size(460, 23);
            this.searchDBFileCombo.TabIndex = 105;
            // 
            // FindProteinDBDlg
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(568, 138);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.btnBrowseSearchDBFile);
            this.Controls.Add(this.searchDbLabel);
            this.Controls.Add(this.searchDBFileCombo);
            this.Controls.Add(this.btnCancel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "FindProteinDBDlg";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Find Search Database File";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnBrowseSearchDBFile;
        private System.Windows.Forms.Label searchDbLabel;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.ComboBox searchDBFileCombo;
    }
}