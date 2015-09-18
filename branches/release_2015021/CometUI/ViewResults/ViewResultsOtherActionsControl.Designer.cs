namespace CometUI.ViewResults
{
    partial class ViewResultsOtherActionsControl
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
            this.otherActionsMainPanel = new System.Windows.Forms.Panel();
            this.fdrCutoffBtn = new System.Windows.Forms.Button();
            this.exportResultsBtn = new System.Windows.Forms.Button();
            this.otherActionsMainPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // otherActionsMainPanel
            // 
            this.otherActionsMainPanel.Controls.Add(this.fdrCutoffBtn);
            this.otherActionsMainPanel.Controls.Add(this.exportResultsBtn);
            this.otherActionsMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.otherActionsMainPanel.Location = new System.Drawing.Point(0, 0);
            this.otherActionsMainPanel.Name = "otherActionsMainPanel";
            this.otherActionsMainPanel.Size = new System.Drawing.Size(1000, 210);
            this.otherActionsMainPanel.TabIndex = 1;
            // 
            // fdrCutoffBtn
            // 
            this.fdrCutoffBtn.Location = new System.Drawing.Point(182, 37);
            this.fdrCutoffBtn.Name = "fdrCutoffBtn";
            this.fdrCutoffBtn.Size = new System.Drawing.Size(125, 35);
            this.fdrCutoffBtn.TabIndex = 4;
            this.fdrCutoffBtn.Text = "&Apply FDR Cutoff...";
            this.fdrCutoffBtn.UseVisualStyleBackColor = true;
            this.fdrCutoffBtn.Click += new System.EventHandler(this.FDRCutoffBtnClick);
            // 
            // exportResultsBtn
            // 
            this.exportResultsBtn.Location = new System.Drawing.Point(24, 37);
            this.exportResultsBtn.Name = "exportResultsBtn";
            this.exportResultsBtn.Size = new System.Drawing.Size(125, 35);
            this.exportResultsBtn.TabIndex = 0;
            this.exportResultsBtn.Text = "&Export Results List...";
            this.exportResultsBtn.UseVisualStyleBackColor = true;
            this.exportResultsBtn.Click += new System.EventHandler(this.ExportResultsBtnClick);
            // 
            // ViewResultsOtherActionsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.otherActionsMainPanel);
            this.Name = "ViewResultsOtherActionsControl";
            this.Size = new System.Drawing.Size(1000, 210);
            this.otherActionsMainPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel otherActionsMainPanel;
        private System.Windows.Forms.Button exportResultsBtn;
        private System.Windows.Forms.Button fdrCutoffBtn;
    }
}
