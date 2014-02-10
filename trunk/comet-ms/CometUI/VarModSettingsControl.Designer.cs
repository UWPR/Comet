namespace CometUI
{
    partial class VarModSettingsControl
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
            this.varModsDataGridView = new System.Windows.Forms.DataGridView();
            this.label1 = new System.Windows.Forms.Label();
            this.varModsResidueCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsMassCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsBinaryModCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsMaxModsPerPeptide = new System.Windows.Forms.DataGridViewTextBoxColumn();
            ((System.ComponentModel.ISupportInitialize)(this.varModsDataGridView)).BeginInit();
            this.SuspendLayout();
            // 
            // varModsDataGridView
            // 
            this.varModsDataGridView.AllowUserToAddRows = false;
            this.varModsDataGridView.AllowUserToDeleteRows = false;
            this.varModsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.varModsDataGridView.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.varModsResidueCol,
            this.varModsMassCol,
            this.varModsBinaryModCol,
            this.varModsMaxModsPerPeptide});
            this.varModsDataGridView.Location = new System.Drawing.Point(23, 35);
            this.varModsDataGridView.Name = "varModsDataGridView";
            this.varModsDataGridView.Size = new System.Drawing.Size(473, 167);
            this.varModsDataGridView.TabIndex = 0;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(23, 16);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(77, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "Variable Mods:";
            // 
            // varModsResidueCol
            // 
            this.varModsResidueCol.HeaderText = "Residue";
            this.varModsResidueCol.Name = "varModsResidueCol";
            // 
            // varModsMassCol
            // 
            this.varModsMassCol.HeaderText = "Mass";
            this.varModsMassCol.Name = "varModsMassCol";
            // 
            // varModsBinaryModCol
            // 
            this.varModsBinaryModCol.HeaderText = "Binary";
            this.varModsBinaryModCol.Name = "varModsBinaryModCol";
            // 
            // varModsMaxModsPerPeptide
            // 
            this.varModsMaxModsPerPeptide.HeaderText = "Max Mods/Peptide";
            this.varModsMaxModsPerPeptide.Name = "varModsMaxModsPerPeptide";
            // 
            // VarModSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.label1);
            this.Controls.Add(this.varModsDataGridView);
            this.Name = "VarModSettingsControl";
            this.Size = new System.Drawing.Size(527, 425);
            ((System.ComponentModel.ISupportInitialize)(this.varModsDataGridView)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.DataGridView varModsDataGridView;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsResidueCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMassCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsBinaryModCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMaxModsPerPeptide;

    }
}
