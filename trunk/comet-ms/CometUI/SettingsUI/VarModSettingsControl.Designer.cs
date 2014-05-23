using CometUI.CommonControls;

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
            this.varModsDataGridView = new System.Windows.Forms.DataGridView();
            this.varModsResidueCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsMassCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.varModsBinaryModCol = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.varModsMaxModsPerPeptide = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.variableNTerminusTextBox = new NumericTextBox();
            this.variableCTerminusTextBox = new NumericTextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.variableNTerminusDistTextBox = new NumericTextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.variableCTerminusDistTextBox = new NumericTextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.maxModsInPeptideTextBox = new System.Windows.Forms.NumericUpDown();
            ((System.ComponentModel.ISupportInitialize)(this.varModsDataGridView)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.maxModsInPeptideTextBox)).BeginInit();
            this.SuspendLayout();
            // 
            // varModsDataGridView
            // 
            this.varModsDataGridView.AllowUserToAddRows = false;
            this.varModsDataGridView.AllowUserToDeleteRows = false;
            this.varModsDataGridView.AllowUserToResizeRows = false;
            this.varModsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.varModsDataGridView.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.varModsResidueCol,
            this.varModsMassCol,
            this.varModsBinaryModCol,
            this.varModsMaxModsPerPeptide});
            this.varModsDataGridView.Location = new System.Drawing.Point(26, 26);
            this.varModsDataGridView.Name = "varModsDataGridView";
            this.varModsDataGridView.Size = new System.Drawing.Size(443, 155);
            this.varModsDataGridView.TabIndex = 0;
            this.varModsDataGridView.CellEndEdit += new System.Windows.Forms.DataGridViewCellEventHandler(this.VarModsDataGridViewCellEndEdit);
            // 
            // varModsResidueCol
            // 
            this.varModsResidueCol.HeaderText = "Residue";
            this.varModsResidueCol.Name = "varModsResidueCol";
            this.varModsResidueCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsResidueCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // varModsMassCol
            // 
            this.varModsMassCol.HeaderText = "Mass Diff";
            this.varModsMassCol.Name = "varModsMassCol";
            this.varModsMassCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsMassCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // varModsBinaryModCol
            // 
            this.varModsBinaryModCol.HeaderText = "Binary Mod";
            this.varModsBinaryModCol.Name = "varModsBinaryModCol";
            this.varModsBinaryModCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            // 
            // varModsMaxModsPerPeptide
            // 
            this.varModsMaxModsPerPeptide.HeaderText = "Max Mods";
            this.varModsMaxModsPerPeptide.Name = "varModsMaxModsPerPeptide";
            this.varModsMaxModsPerPeptide.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.varModsMaxModsPerPeptide.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // variableNTerminusTextBox
            // 
            this.variableNTerminusTextBox.AllowDecimal = true;
            this.variableNTerminusTextBox.AllowSpace = false;
            this.variableNTerminusTextBox.Location = new System.Drawing.Point(89, 214);
            this.variableNTerminusTextBox.Name = "variableNTerminusTextBox";
            this.variableNTerminusTextBox.Size = new System.Drawing.Size(63, 20);
            this.variableNTerminusTextBox.TabIndex = 7;
            // 
            // variableCTerminusTextBox
            // 
            this.variableCTerminusTextBox.AllowDecimal = true;
            this.variableCTerminusTextBox.AllowSpace = false;
            this.variableCTerminusTextBox.Location = new System.Drawing.Point(89, 240);
            this.variableCTerminusTextBox.Name = "variableCTerminusTextBox";
            this.variableCTerminusTextBox.Size = new System.Drawing.Size(63, 20);
            this.variableCTerminusTextBox.TabIndex = 8;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(23, 217);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(60, 13);
            this.label2.TabIndex = 9;
            this.label2.Text = "N-terminus:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(23, 243);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(59, 13);
            this.label3.TabIndex = 10;
            this.label3.Text = "C-terminus:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(171, 217);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(52, 13);
            this.label4.TabIndex = 12;
            this.label4.Text = "Distance:";
            // 
            // variableNTerminusDistTextBox
            // 
            this.variableNTerminusDistTextBox.AllowDecimal = false;
            this.variableNTerminusDistTextBox.AllowSpace = false;
            this.variableNTerminusDistTextBox.Location = new System.Drawing.Point(229, 214);
            this.variableNTerminusDistTextBox.Name = "variableNTerminusDistTextBox";
            this.variableNTerminusDistTextBox.Size = new System.Drawing.Size(63, 20);
            this.variableNTerminusDistTextBox.TabIndex = 11;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(171, 243);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(52, 13);
            this.label5.TabIndex = 14;
            this.label5.Text = "Distance:";
            // 
            // variableCTerminusDistTextBox
            // 
            this.variableCTerminusDistTextBox.AllowDecimal = false;
            this.variableCTerminusDistTextBox.AllowSpace = false;
            this.variableCTerminusDistTextBox.Location = new System.Drawing.Point(229, 240);
            this.variableCTerminusDistTextBox.Name = "variableCTerminusDistTextBox";
            this.variableCTerminusDistTextBox.Size = new System.Drawing.Size(63, 20);
            this.variableCTerminusDistTextBox.TabIndex = 13;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(362, 217);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(107, 13);
            this.label6.TabIndex = 15;
            this.label6.Text = "Max mods in peptide:";
            // 
            // maxModsInPeptideTextBox
            // 
            this.maxModsInPeptideTextBox.Location = new System.Drawing.Point(365, 233);
            this.maxModsInPeptideTextBox.Maximum = new decimal(new int[] {
            20,
            0,
            0,
            0});
            this.maxModsInPeptideTextBox.Name = "maxModsInPeptideTextBox";
            this.maxModsInPeptideTextBox.Size = new System.Drawing.Size(104, 20);
            this.maxModsInPeptideTextBox.TabIndex = 32;
            // 
            // VarModSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.varModsDataGridView);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.maxModsInPeptideTextBox);
            this.Controls.Add(this.variableCTerminusTextBox);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.variableNTerminusTextBox);
            this.Controls.Add(this.variableNTerminusDistTextBox);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.variableCTerminusDistTextBox);
            this.Controls.Add(this.label4);
            this.Name = "VarModSettingsControl";
            this.Size = new System.Drawing.Size(527, 450);
            ((System.ComponentModel.ISupportInitialize)(this.varModsDataGridView)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.maxModsInPeptideTextBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.DataGridView varModsDataGridView;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.NumericUpDown maxModsInPeptideTextBox;
        private NumericTextBox variableNTerminusTextBox;
        private NumericTextBox variableCTerminusTextBox;
        private NumericTextBox variableNTerminusDistTextBox;
        private NumericTextBox variableCTerminusDistTextBox;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsResidueCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMassCol;
        private System.Windows.Forms.DataGridViewCheckBoxColumn varModsBinaryModCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMaxModsPerPeptide;

    }
}
