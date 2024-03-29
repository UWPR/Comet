﻿namespace CometUI.ViewResults
{
    partial class FDRDlg
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(FDRDlg));
            this.cancelFDRCutoffBtn = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.fdrCutoffTextBox = new CometUI.SharedUI.NumericTextBox();
            this.applyFDRCutoffBtn = new System.Windows.Forms.Button();
            this.showDecoyHitsCheckBox = new System.Windows.Forms.CheckBox();
            this.SuspendLayout();
            // 
            // cancelFDRCutoffBtn
            // 
            this.cancelFDRCutoffBtn.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.cancelFDRCutoffBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelFDRCutoffBtn.Location = new System.Drawing.Point(93, 139);
            this.cancelFDRCutoffBtn.Name = "cancelFDRCutoffBtn";
            this.cancelFDRCutoffBtn.Size = new System.Drawing.Size(75, 23);
            this.cancelFDRCutoffBtn.TabIndex = 4;
            this.cancelFDRCutoffBtn.Text = "Cancel";
            this.cancelFDRCutoffBtn.UseVisualStyleBackColor = true;
            this.cancelFDRCutoffBtn.Click += new System.EventHandler(this.CancelFDRCutoffBtnClick);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(9, 25);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(115, 13);
            this.label1.TabIndex = 6;
            this.label1.Text = "%FDR (q-value) Cutoff:";
            // 
            // fdrCutoffTextBox
            // 
            this.fdrCutoffTextBox.AllowDecimal = true;
            this.fdrCutoffTextBox.AllowGroupSeparator = false;
            this.fdrCutoffTextBox.AllowNegative = false;
            this.fdrCutoffTextBox.AllowSpace = false;
            this.fdrCutoffTextBox.Location = new System.Drawing.Point(12, 41);
            this.fdrCutoffTextBox.Name = "fdrCutoffTextBox";
            this.fdrCutoffTextBox.Size = new System.Drawing.Size(112, 20);
            this.fdrCutoffTextBox.TabIndex = 1;
            this.fdrCutoffTextBox.Text = "100";
            // 
            // applyFDRCutoffBtn
            // 
            this.applyFDRCutoffBtn.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.applyFDRCutoffBtn.Location = new System.Drawing.Point(12, 139);
            this.applyFDRCutoffBtn.Name = "applyFDRCutoffBtn";
            this.applyFDRCutoffBtn.Size = new System.Drawing.Size(75, 23);
            this.applyFDRCutoffBtn.TabIndex = 3;
            this.applyFDRCutoffBtn.Text = "Apply";
            this.applyFDRCutoffBtn.UseVisualStyleBackColor = true;
            this.applyFDRCutoffBtn.Click += new System.EventHandler(this.ApplyFDRCutoffBtnClick);
            // 
            // showDecoyHitsCheckBox
            // 
            this.showDecoyHitsCheckBox.AutoSize = true;
            this.showDecoyHitsCheckBox.Location = new System.Drawing.Point(12, 87);
            this.showDecoyHitsCheckBox.Name = "showDecoyHitsCheckBox";
            this.showDecoyHitsCheckBox.Size = new System.Drawing.Size(104, 17);
            this.showDecoyHitsCheckBox.TabIndex = 2;
            this.showDecoyHitsCheckBox.Text = "Show decoy hits";
            this.showDecoyHitsCheckBox.UseVisualStyleBackColor = true;
            // 
            // FDRDlg
            // 
            this.AcceptButton = this.applyFDRCutoffBtn;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.cancelFDRCutoffBtn;
            this.ClientSize = new System.Drawing.Size(247, 174);
            this.Controls.Add(this.showDecoyHitsCheckBox);
            this.Controls.Add(this.applyFDRCutoffBtn);
            this.Controls.Add(this.cancelFDRCutoffBtn);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.fdrCutoffTextBox);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "FDRDlg";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "FDR Cutoff";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button cancelFDRCutoffBtn;
        private System.Windows.Forms.Label label1;
        private SharedUI.NumericTextBox fdrCutoffTextBox;
        private System.Windows.Forms.Button applyFDRCutoffBtn;
        private System.Windows.Forms.CheckBox showDecoyHitsCheckBox;
    }
}