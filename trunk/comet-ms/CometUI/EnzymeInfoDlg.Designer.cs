namespace CometUI
{
    partial class EnzymeInfoDlg
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
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            this.enzymeInfoDataGridView = new System.Windows.Forms.DataGridView();
            this.EnzymeNumber = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.EnzymeName = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.EnzymeOffset = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.EnzymeBreakAA = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.EnzymeNoBreakAA = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.enzymeInfoMainSplitContainer = new System.Windows.Forms.SplitContainer();
            this.enzymeInfoOKButton = new System.Windows.Forms.Button();
            this.enzymeInfoCancelButton = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.enzymeInfoDataGridView)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.enzymeInfoMainSplitContainer)).BeginInit();
            this.enzymeInfoMainSplitContainer.Panel1.SuspendLayout();
            this.enzymeInfoMainSplitContainer.Panel2.SuspendLayout();
            this.enzymeInfoMainSplitContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // enzymeInfoDataGridView
            // 
            dataGridViewCellStyle1.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle1.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle1.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle1.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle1.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle1.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.enzymeInfoDataGridView.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle1;
            this.enzymeInfoDataGridView.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.enzymeInfoDataGridView.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.EnzymeNumber,
            this.EnzymeName,
            this.EnzymeOffset,
            this.EnzymeBreakAA,
            this.EnzymeNoBreakAA});
            dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle2.BackColor = System.Drawing.SystemColors.Window;
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle2.ForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle2.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle2.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle2.WrapMode = System.Windows.Forms.DataGridViewTriState.False;
            this.enzymeInfoDataGridView.DefaultCellStyle = dataGridViewCellStyle2;
            this.enzymeInfoDataGridView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.enzymeInfoDataGridView.Location = new System.Drawing.Point(0, 0);
            this.enzymeInfoDataGridView.MultiSelect = false;
            this.enzymeInfoDataGridView.Name = "enzymeInfoDataGridView";
            dataGridViewCellStyle3.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle3.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle3.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle3.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle3.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle3.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.enzymeInfoDataGridView.RowHeadersDefaultCellStyle = dataGridViewCellStyle3;
            this.enzymeInfoDataGridView.Size = new System.Drawing.Size(417, 237);
            this.enzymeInfoDataGridView.TabIndex = 0;
            this.enzymeInfoDataGridView.CellValueChanged += new System.Windows.Forms.DataGridViewCellEventHandler(this.EnzymeInfoDataGridViewCellValueChanged);
            // 
            // EnzymeNumber
            // 
            this.EnzymeNumber.HeaderText = "Number";
            this.EnzymeNumber.Name = "EnzymeNumber";
            this.EnzymeNumber.Resizable = System.Windows.Forms.DataGridViewTriState.True;
            this.EnzymeNumber.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.EnzymeNumber.Width = 75;
            // 
            // EnzymeName
            // 
            this.EnzymeName.HeaderText = "Name";
            this.EnzymeName.Name = "EnzymeName";
            this.EnzymeName.Width = 75;
            // 
            // EnzymeOffset
            // 
            this.EnzymeOffset.HeaderText = "Offset";
            this.EnzymeOffset.Name = "EnzymeOffset";
            this.EnzymeOffset.Width = 75;
            // 
            // EnzymeBreakAA
            // 
            this.EnzymeBreakAA.HeaderText = "Break AA";
            this.EnzymeBreakAA.Name = "EnzymeBreakAA";
            this.EnzymeBreakAA.Width = 75;
            // 
            // EnzymeNoBreakAA
            // 
            this.EnzymeNoBreakAA.HeaderText = "No Break AA";
            this.EnzymeNoBreakAA.Name = "EnzymeNoBreakAA";
            this.EnzymeNoBreakAA.Width = 75;
            // 
            // enzymeInfoMainSplitContainer
            // 
            this.enzymeInfoMainSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.enzymeInfoMainSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this.enzymeInfoMainSplitContainer.IsSplitterFixed = true;
            this.enzymeInfoMainSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.enzymeInfoMainSplitContainer.Name = "enzymeInfoMainSplitContainer";
            this.enzymeInfoMainSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // enzymeInfoMainSplitContainer.Panel1
            // 
            this.enzymeInfoMainSplitContainer.Panel1.Controls.Add(this.enzymeInfoDataGridView);
            // 
            // enzymeInfoMainSplitContainer.Panel2
            // 
            this.enzymeInfoMainSplitContainer.Panel2.Controls.Add(this.enzymeInfoOKButton);
            this.enzymeInfoMainSplitContainer.Panel2.Controls.Add(this.enzymeInfoCancelButton);
            this.enzymeInfoMainSplitContainer.Size = new System.Drawing.Size(417, 276);
            this.enzymeInfoMainSplitContainer.SplitterDistance = 237;
            this.enzymeInfoMainSplitContainer.TabIndex = 1;
            // 
            // enzymeInfoOKButton
            // 
            this.enzymeInfoOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.enzymeInfoOKButton.Location = new System.Drawing.Point(249, 3);
            this.enzymeInfoOKButton.Name = "enzymeInfoOKButton";
            this.enzymeInfoOKButton.Size = new System.Drawing.Size(75, 23);
            this.enzymeInfoOKButton.TabIndex = 1;
            this.enzymeInfoOKButton.Text = "OK";
            this.enzymeInfoOKButton.UseVisualStyleBackColor = true;
            this.enzymeInfoOKButton.Click += new System.EventHandler(this.EnzymeInfoOkButtonClick);
            // 
            // enzymeInfoCancelButton
            // 
            this.enzymeInfoCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.enzymeInfoCancelButton.Location = new System.Drawing.Point(330, 3);
            this.enzymeInfoCancelButton.Name = "enzymeInfoCancelButton";
            this.enzymeInfoCancelButton.Size = new System.Drawing.Size(75, 23);
            this.enzymeInfoCancelButton.TabIndex = 2;
            this.enzymeInfoCancelButton.Text = "Cancel";
            this.enzymeInfoCancelButton.UseVisualStyleBackColor = true;
            this.enzymeInfoCancelButton.Click += new System.EventHandler(this.EnzymeInfoCancelButtonClick);
            // 
            // EnzymeInfoDlg
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(417, 276);
            this.Controls.Add(this.enzymeInfoMainSplitContainer);
            this.MaximizeBox = false;
            this.Name = "EnzymeInfoDlg";
            this.Text = "Enzyme Info";
            ((System.ComponentModel.ISupportInitialize)(this.enzymeInfoDataGridView)).EndInit();
            this.enzymeInfoMainSplitContainer.Panel1.ResumeLayout(false);
            this.enzymeInfoMainSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.enzymeInfoMainSplitContainer)).EndInit();
            this.enzymeInfoMainSplitContainer.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.DataGridView enzymeInfoDataGridView;
        private System.Windows.Forms.SplitContainer enzymeInfoMainSplitContainer;
        private System.Windows.Forms.DataGridViewTextBoxColumn EnzymeNumber;
        private System.Windows.Forms.DataGridViewTextBoxColumn EnzymeName;
        private System.Windows.Forms.DataGridViewTextBoxColumn EnzymeOffset;
        private System.Windows.Forms.DataGridViewTextBoxColumn EnzymeBreakAA;
        private System.Windows.Forms.DataGridViewTextBoxColumn EnzymeNoBreakAA;
        private System.Windows.Forms.Button enzymeInfoOKButton;
        private System.Windows.Forms.Button enzymeInfoCancelButton;
    }
}