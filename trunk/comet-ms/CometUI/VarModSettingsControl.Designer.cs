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
            this.variableNTerminusTextBox = new System.Windows.Forms.TextBox();
            this.variableCTerminusTextBox = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.variableNTerminusDistTextBox = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.variableCTerminusDistTextBox = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.maxModsInPeptideCombo = new System.Windows.Forms.ComboBox();
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
            this.varModsDataGridView.AllowUserToResizeColumns = false;
            this.varModsDataGridView.AllowUserToResizeRows = false;
            this.varModsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.varModsDataGridView.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.varModsResidueCol,
            this.varModsMassCol,
            this.varModsBinaryModCol,
            this.varModsMaxModsPerPeptide});
            this.varModsDataGridView.Location = new System.Drawing.Point(25, 32);
            this.varModsDataGridView.Name = "varModsDataGridView";
            this.varModsDataGridView.Size = new System.Drawing.Size(443, 155);
            this.varModsDataGridView.TabIndex = 0;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(22, 16);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(77, 13);
            this.label1.TabIndex = 1;
            this.label1.Text = "Variable Mods:";
            // 
            // variableNTerminusTextBox
            // 
            this.variableNTerminusTextBox.Location = new System.Drawing.Point(88, 212);
            this.variableNTerminusTextBox.Name = "variableNTerminusTextBox";
            this.variableNTerminusTextBox.Size = new System.Drawing.Size(84, 20);
            this.variableNTerminusTextBox.TabIndex = 7;
            // 
            // variableCTerminusTextBox
            // 
            this.variableCTerminusTextBox.Location = new System.Drawing.Point(88, 244);
            this.variableCTerminusTextBox.Name = "variableCTerminusTextBox";
            this.variableCTerminusTextBox.Size = new System.Drawing.Size(84, 20);
            this.variableCTerminusTextBox.TabIndex = 8;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(22, 215);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(60, 13);
            this.label2.TabIndex = 9;
            this.label2.Text = "N-terminus:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(22, 247);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(59, 13);
            this.label3.TabIndex = 10;
            this.label3.Text = "C-terminus:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(197, 215);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(52, 13);
            this.label4.TabIndex = 12;
            this.label4.Text = "Distance:";
            // 
            // variableNTerminusDistTextBox
            // 
            this.variableNTerminusDistTextBox.Location = new System.Drawing.Point(255, 212);
            this.variableNTerminusDistTextBox.Name = "variableNTerminusDistTextBox";
            this.variableNTerminusDistTextBox.Size = new System.Drawing.Size(84, 20);
            this.variableNTerminusDistTextBox.TabIndex = 11;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(197, 247);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(52, 13);
            this.label5.TabIndex = 14;
            this.label5.Text = "Distance:";
            // 
            // variableCTerminusDistTextBox
            // 
            this.variableCTerminusDistTextBox.Location = new System.Drawing.Point(255, 244);
            this.variableCTerminusDistTextBox.Name = "variableCTerminusDistTextBox";
            this.variableCTerminusDistTextBox.Size = new System.Drawing.Size(84, 20);
            this.variableCTerminusDistTextBox.TabIndex = 13;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(22, 285);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(107, 13);
            this.label6.TabIndex = 15;
            this.label6.Text = "Max mods in peptide:";
            // 
            // maxModsInPeptideCombo
            // 
            this.maxModsInPeptideCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.maxModsInPeptideCombo.FormattingEnabled = true;
            this.maxModsInPeptideCombo.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4",
            "5"});
            this.maxModsInPeptideCombo.Location = new System.Drawing.Point(25, 301);
            this.maxModsInPeptideCombo.Name = "maxModsInPeptideCombo";
            this.maxModsInPeptideCombo.Size = new System.Drawing.Size(100, 21);
            this.maxModsInPeptideCombo.TabIndex = 16;
            // 
            // varModsResidueCol
            // 
            this.varModsResidueCol.HeaderText = "Residue";
            this.varModsResidueCol.Name = "varModsResidueCol";
            this.varModsResidueCol.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.varModsResidueCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // varModsMassCol
            // 
            this.varModsMassCol.HeaderText = "Mass";
            this.varModsMassCol.Name = "varModsMassCol";
            this.varModsMassCol.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.varModsMassCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // varModsBinaryModCol
            // 
            this.varModsBinaryModCol.HeaderText = "Binary Mod";
            this.varModsBinaryModCol.Name = "varModsBinaryModCol";
            this.varModsBinaryModCol.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.varModsBinaryModCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // varModsMaxModsPerPeptide
            // 
            this.varModsMaxModsPerPeptide.HeaderText = "Max Mods";
            this.varModsMaxModsPerPeptide.Name = "varModsMaxModsPerPeptide";
            this.varModsMaxModsPerPeptide.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.varModsMaxModsPerPeptide.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // VarModSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.maxModsInPeptideCombo);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.variableCTerminusDistTextBox);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.variableNTerminusDistTextBox);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.variableCTerminusTextBox);
            this.Controls.Add(this.variableNTerminusTextBox);
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
        private System.Windows.Forms.TextBox variableNTerminusTextBox;
        private System.Windows.Forms.TextBox variableCTerminusTextBox;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox variableNTerminusDistTextBox;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox variableCTerminusDistTextBox;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.ComboBox maxModsInPeptideCombo;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsResidueCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMassCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsBinaryModCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn varModsMaxModsPerPeptide;

    }
}
