using CometUI.CustomControls;

namespace CometUI.SettingsUI
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
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            this.varModsDataGridView = new System.Windows.Forms.DataGridView();
            this.label6 = new System.Windows.Forms.Label();
            this.maxModsInPeptideTextBox = new System.Windows.Forms.NumericUpDown();
            this.varModsMassCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsResidueCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsBinaryModCol = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.varModsMaxModsPerPeptide = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsTermDistance = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsWhichTerm = new System.Windows.Forms.DataGridViewComboBoxColumn();
            ((System.ComponentModel.ISupportInitialize)(this.varModsDataGridView)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.maxModsInPeptideTextBox)).BeginInit();
            this.SuspendLayout();
            // 
            // varModsDataGridView
            // 
            this.varModsDataGridView.AllowUserToAddRows = false;
            this.varModsDataGridView.AllowUserToDeleteRows = false;
            this.varModsDataGridView.AllowUserToResizeRows = false;
            dataGridViewCellStyle1.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle1.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle1.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle1.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle1.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle1.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsDataGridView.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle1;
            this.varModsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.varModsDataGridView.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.varModsMassCol,
            this.varModsResidueCol,
            this.varModsBinaryModCol,
            this.varModsMaxModsPerPeptide,
            this.varModsTermDistance,
            this.varModsWhichTerm});
            dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle2.BackColor = System.Drawing.SystemColors.Window;
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle2.ForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle2.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle2.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle2.WrapMode = System.Windows.Forms.DataGridViewTriState.False;
            this.varModsDataGridView.DefaultCellStyle = dataGridViewCellStyle2;
            this.varModsDataGridView.Location = new System.Drawing.Point(26, 26);
            this.varModsDataGridView.Name = "varModsDataGridView";
            dataGridViewCellStyle3.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle3.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle3.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle3.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle3.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle3.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsDataGridView.RowHeadersDefaultCellStyle = dataGridViewCellStyle3;
            this.varModsDataGridView.Size = new System.Drawing.Size(443, 221);
            this.varModsDataGridView.TabIndex = 0;
            this.varModsDataGridView.CellEndEdit += new System.Windows.Forms.DataGridViewCellEventHandler(this.VarModsDataGridViewCellEndEdit);
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(23, 269);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(107, 13);
            this.label6.TabIndex = 15;
            this.label6.Text = "Max mods in peptide:";
            // 
            // maxModsInPeptideTextBox
            // 
            this.maxModsInPeptideTextBox.Location = new System.Drawing.Point(26, 285);
            this.maxModsInPeptideTextBox.Maximum = new decimal(new int[] {
            20,
            0,
            0,
            0});
            this.maxModsInPeptideTextBox.Name = "maxModsInPeptideTextBox";
            this.maxModsInPeptideTextBox.Size = new System.Drawing.Size(104, 20);
            this.maxModsInPeptideTextBox.TabIndex = 32;
            // 
            // varModsMassCol
            // 
            this.varModsMassCol.HeaderText = "Mass Diff";
            this.varModsMassCol.Name = "varModsMassCol";
            this.varModsMassCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsMassCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.varModsMassCol.Width = 65;
            // 
            // varModsResidueCol
            // 
            this.varModsResidueCol.HeaderText = "Residue";
            this.varModsResidueCol.Name = "varModsResidueCol";
            this.varModsResidueCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsResidueCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.varModsResidueCol.Width = 65;
            // 
            // varModsBinaryModCol
            // 
            this.varModsBinaryModCol.HeaderText = "Bin Mod";
            this.varModsBinaryModCol.Name = "varModsBinaryModCol";
            this.varModsBinaryModCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsBinaryModCol.Width = 65;
            // 
            // varModsMaxModsPerPeptide
            // 
            this.varModsMaxModsPerPeptide.HeaderText = "Max Mods";
            this.varModsMaxModsPerPeptide.Name = "varModsMaxModsPerPeptide";
            this.varModsMaxModsPerPeptide.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsMaxModsPerPeptide.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.varModsMaxModsPerPeptide.Width = 65;
            // 
            // varModsTermDistance
            // 
            this.varModsTermDistance.HeaderText = "Term Dist";
            this.varModsTermDistance.Name = "varModsTermDistance";
            this.varModsTermDistance.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.varModsTermDistance.Width = 65;
            // 
            // varModsWhichTerm
            // 
            this.varModsWhichTerm.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
            this.varModsWhichTerm.HeaderText = "Which Term";
            this.varModsWhichTerm.Items.AddRange(new object[] {
            "0",
            "1"});
            this.varModsWhichTerm.Name = "varModsWhichTerm";
            this.varModsWhichTerm.Width = 74;
            // 
            // VarModSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.varModsDataGridView);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.maxModsInPeptideTextBox);
            this.Name = "VarModSettingsControl";
            this.Size = new System.Drawing.Size(527, 450);
            ((System.ComponentModel.ISupportInitialize)(this.varModsDataGridView)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.maxModsInPeptideTextBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.DataGridView varModsDataGridView;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.NumericUpDown maxModsInPeptideTextBox;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMassCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsResidueCol;
        private System.Windows.Forms.DataGridViewCheckBoxColumn varModsBinaryModCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMaxModsPerPeptide;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsTermDistance;
        private System.Windows.Forms.DataGridViewComboBoxColumn varModsWhichTerm;

    }
}
