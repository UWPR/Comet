using CometUI.CommonControls;

namespace CometUI.SettingsUI
{
    partial class StaticModSettingsControl
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
            this.staticModsDataGridView = new System.Windows.Forms.DataGridView();
            this.staticModsNameCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.staticModsResidueCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.staticModsMassDiffCol = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.staticCTermPeptideTextBox = new NumericTextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.staticNTermPeptideTextBox = new NumericTextBox();
            this.staticCTermProteinTextBox = new NumericTextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.staticNTermProteinTextBox = new NumericTextBox();
            ((System.ComponentModel.ISupportInitialize)(this.staticModsDataGridView)).BeginInit();
            this.SuspendLayout();
            // 
            // staticModsDataGridView
            // 
            this.staticModsDataGridView.AllowUserToAddRows = false;
            this.staticModsDataGridView.AllowUserToDeleteRows = false;
            this.staticModsDataGridView.AllowUserToResizeRows = false;
            this.staticModsDataGridView.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.staticModsDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.staticModsDataGridView.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.staticModsNameCol,
            this.staticModsResidueCol,
            this.staticModsMassDiffCol});
            this.staticModsDataGridView.Location = new System.Drawing.Point(26, 26);
            this.staticModsDataGridView.Name = "staticModsDataGridView";
            this.staticModsDataGridView.Size = new System.Drawing.Size(432, 287);
            this.staticModsDataGridView.TabIndex = 1;
            this.staticModsDataGridView.CellEndEdit += new System.Windows.Forms.DataGridViewCellEventHandler(this.StaticModsDataGridViewCellEndEdit);
            // 
            // staticModsNameCol
            // 
            this.staticModsNameCol.HeaderText = "Name";
            this.staticModsNameCol.Name = "staticModsNameCol";
            this.staticModsNameCol.ReadOnly = true;
            this.staticModsNameCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.staticModsNameCol.Width = 124;
            // 
            // staticModsResidueCol
            // 
            this.staticModsResidueCol.HeaderText = "Residue";
            this.staticModsResidueCol.Name = "staticModsResidueCol";
            this.staticModsResidueCol.ReadOnly = true;
            this.staticModsResidueCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.staticModsResidueCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.staticModsResidueCol.Width = 124;
            // 
            // staticModsMassDiffCol
            // 
            this.staticModsMassDiffCol.HeaderText = "Mass Diff";
            this.staticModsMassDiffCol.Name = "staticModsMassDiffCol";
            this.staticModsMassDiffCol.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.staticModsMassDiffCol.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.staticModsMassDiffCol.Width = 124;
            // 
            // staticCTermPeptideTextBox
            // 
            this.staticCTermPeptideTextBox.AllowSpace = false;
            this.staticCTermPeptideTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.staticCTermPeptideTextBox.Location = new System.Drawing.Point(128, 372);
            this.staticCTermPeptideTextBox.Name = "staticCTermPeptideTextBox";
            this.staticCTermPeptideTextBox.Size = new System.Drawing.Size(86, 20);
            this.staticCTermPeptideTextBox.TabIndex = 12;
            // 
            // label2
            // 
            this.label2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(23, 349);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(99, 13);
            this.label2.TabIndex = 13;
            this.label2.Text = "N-terminus Peptide:";
            // 
            // label3
            // 
            this.label3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(23, 375);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(98, 13);
            this.label3.TabIndex = 14;
            this.label3.Text = "C-terminus Peptide:";
            // 
            // staticNTermPeptideTextBox
            // 
            this.staticNTermPeptideTextBox.AllowSpace = false;
            this.staticNTermPeptideTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.staticNTermPeptideTextBox.Location = new System.Drawing.Point(128, 346);
            this.staticNTermPeptideTextBox.Name = "staticNTermPeptideTextBox";
            this.staticNTermPeptideTextBox.Size = new System.Drawing.Size(86, 20);
            this.staticNTermPeptideTextBox.TabIndex = 11;
            // 
            // staticCTermProteinTextBox
            // 
            this.staticCTermProteinTextBox.AllowSpace = false;
            this.staticCTermProteinTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.staticCTermProteinTextBox.Location = new System.Drawing.Point(372, 372);
            this.staticCTermProteinTextBox.Name = "staticCTermProteinTextBox";
            this.staticCTermProteinTextBox.Size = new System.Drawing.Size(86, 20);
            this.staticCTermProteinTextBox.TabIndex = 16;
            // 
            // label1
            // 
            this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(270, 348);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(96, 13);
            this.label1.TabIndex = 17;
            this.label1.Text = "N-terminus Protein:";
            // 
            // label4
            // 
            this.label4.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(270, 375);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(95, 13);
            this.label4.TabIndex = 18;
            this.label4.Text = "C-terminus Protein:";
            // 
            // staticNTermProteinTextBox
            // 
            this.staticNTermProteinTextBox.AllowSpace = false;
            this.staticNTermProteinTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.staticNTermProteinTextBox.Location = new System.Drawing.Point(372, 345);
            this.staticNTermProteinTextBox.Name = "staticNTermProteinTextBox";
            this.staticNTermProteinTextBox.Size = new System.Drawing.Size(86, 20);
            this.staticNTermProteinTextBox.TabIndex = 15;
            // 
            // StaticModSettingsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.staticCTermProteinTextBox);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.staticNTermProteinTextBox);
            this.Controls.Add(this.staticCTermPeptideTextBox);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.staticNTermPeptideTextBox);
            this.Controls.Add(this.staticModsDataGridView);
            this.Name = "StaticModSettingsControl";
            this.Size = new System.Drawing.Size(527, 450);
            ((System.ComponentModel.ISupportInitialize)(this.staticModsDataGridView)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.DataGridView staticModsDataGridView;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.DataGridViewTextBoxColumn staticModsNameCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn staticModsResidueCol;
        private System.Windows.Forms.DataGridViewTextBoxColumn staticModsMassDiffCol;
        private NumericTextBox staticCTermPeptideTextBox;
        private NumericTextBox staticNTermPeptideTextBox;
        private NumericTextBox staticCTermProteinTextBox;
        private NumericTextBox staticNTermProteinTextBox;

    }
}
