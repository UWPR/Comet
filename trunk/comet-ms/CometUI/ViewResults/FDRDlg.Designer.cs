namespace CometUI.ViewResults
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
            this.SuspendLayout();
            // 
            // cancelFDRCutoffBtn
            // 
            this.cancelFDRCutoffBtn.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.cancelFDRCutoffBtn.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.cancelFDRCutoffBtn.Location = new System.Drawing.Point(179, 85);
            this.cancelFDRCutoffBtn.Name = "cancelFDRCutoffBtn";
            this.cancelFDRCutoffBtn.Size = new System.Drawing.Size(75, 23);
            this.cancelFDRCutoffBtn.TabIndex = 3;
            this.cancelFDRCutoffBtn.Text = "Cancel";
            this.cancelFDRCutoffBtn.UseVisualStyleBackColor = true;
            this.cancelFDRCutoffBtn.Click += new System.EventHandler(this.CancelFDRCutoffBtnClick);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 25);
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
            this.fdrCutoffTextBox.Location = new System.Drawing.Point(130, 22);
            this.fdrCutoffTextBox.Name = "fdrCutoffTextBox";
            this.fdrCutoffTextBox.Size = new System.Drawing.Size(124, 20);
            this.fdrCutoffTextBox.TabIndex = 1;
            this.fdrCutoffTextBox.Text = "0.0";
            // 
            // applyFDRCutoffBtn
            // 
            this.applyFDRCutoffBtn.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.applyFDRCutoffBtn.Location = new System.Drawing.Point(98, 85);
            this.applyFDRCutoffBtn.Name = "applyFDRCutoffBtn";
            this.applyFDRCutoffBtn.Size = new System.Drawing.Size(75, 23);
            this.applyFDRCutoffBtn.TabIndex = 2;
            this.applyFDRCutoffBtn.Text = "Apply";
            this.applyFDRCutoffBtn.UseVisualStyleBackColor = true;
            this.applyFDRCutoffBtn.Click += new System.EventHandler(this.ApplyFDRCutoffBtnClick);
            // 
            // FDRDlg
            // 
            this.AcceptButton = this.applyFDRCutoffBtn;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.cancelFDRCutoffBtn;
            this.ClientSize = new System.Drawing.Size(272, 120);
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
    }
}