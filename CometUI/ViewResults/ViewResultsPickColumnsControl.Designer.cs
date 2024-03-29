﻿namespace CometUI.ViewResults
{
    partial class ViewResultsPickColumnsControl
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
            this.pickColumnsMainPanel = new System.Windows.Forms.Panel();
            this.btnUpdateResults = new System.Windows.Forms.Button();
            this.btnMoveDown = new System.Windows.Forms.Button();
            this.btnMoveUp = new System.Windows.Forms.Button();
            this.btnMoveToShowColumns = new System.Windows.Forms.Button();
            this.btnMoveToHideColumns = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.hiddenColumnsLabel = new System.Windows.Forms.Label();
            this.showColumnsListBox = new System.Windows.Forms.ListBox();
            this.hiddenColumnsListBox = new System.Windows.Forms.ListBox();
            this.pickColumnsMainPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // pickColumnsMainPanel
            // 
            this.pickColumnsMainPanel.Controls.Add(this.btnUpdateResults);
            this.pickColumnsMainPanel.Controls.Add(this.btnMoveDown);
            this.pickColumnsMainPanel.Controls.Add(this.btnMoveUp);
            this.pickColumnsMainPanel.Controls.Add(this.btnMoveToShowColumns);
            this.pickColumnsMainPanel.Controls.Add(this.btnMoveToHideColumns);
            this.pickColumnsMainPanel.Controls.Add(this.label1);
            this.pickColumnsMainPanel.Controls.Add(this.hiddenColumnsLabel);
            this.pickColumnsMainPanel.Controls.Add(this.showColumnsListBox);
            this.pickColumnsMainPanel.Controls.Add(this.hiddenColumnsListBox);
            this.pickColumnsMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pickColumnsMainPanel.Location = new System.Drawing.Point(0, 0);
            this.pickColumnsMainPanel.Name = "pickColumnsMainPanel";
            this.pickColumnsMainPanel.Size = new System.Drawing.Size(1000, 210);
            this.pickColumnsMainPanel.TabIndex = 0;
            // 
            // btnUpdateResults
            // 
            this.btnUpdateResults.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnUpdateResults.Location = new System.Drawing.Point(908, 163);
            this.btnUpdateResults.Name = "btnUpdateResults";
            this.btnUpdateResults.Size = new System.Drawing.Size(75, 23);
            this.btnUpdateResults.TabIndex = 52;
            this.btnUpdateResults.Text = "&Update";
            this.btnUpdateResults.UseVisualStyleBackColor = true;
            this.btnUpdateResults.Click += new System.EventHandler(this.BtnUpdateResultsClick);
            // 
            // btnMoveDown
            // 
            this.btnMoveDown.Location = new System.Drawing.Point(561, 84);
            this.btnMoveDown.Name = "btnMoveDown";
            this.btnMoveDown.Size = new System.Drawing.Size(75, 23);
            this.btnMoveDown.TabIndex = 51;
            this.btnMoveDown.Text = "Down";
            this.btnMoveDown.UseVisualStyleBackColor = true;
            this.btnMoveDown.Click += new System.EventHandler(this.BtnMoveDownClick);
            // 
            // btnMoveUp
            // 
            this.btnMoveUp.Location = new System.Drawing.Point(561, 45);
            this.btnMoveUp.Name = "btnMoveUp";
            this.btnMoveUp.Size = new System.Drawing.Size(75, 23);
            this.btnMoveUp.TabIndex = 50;
            this.btnMoveUp.Text = "Up";
            this.btnMoveUp.UseVisualStyleBackColor = true;
            this.btnMoveUp.Click += new System.EventHandler(this.BtnMoveUpClick);
            // 
            // btnMoveToShowColumns
            // 
            this.btnMoveToShowColumns.Location = new System.Drawing.Point(247, 45);
            this.btnMoveToShowColumns.Name = "btnMoveToShowColumns";
            this.btnMoveToShowColumns.Size = new System.Drawing.Size(75, 23);
            this.btnMoveToShowColumns.TabIndex = 49;
            this.btnMoveToShowColumns.Text = ">>";
            this.btnMoveToShowColumns.UseVisualStyleBackColor = true;
            this.btnMoveToShowColumns.Click += new System.EventHandler(this.BtnMoveToShowColumnsClick);
            // 
            // btnMoveToHideColumns
            // 
            this.btnMoveToHideColumns.Location = new System.Drawing.Point(247, 84);
            this.btnMoveToHideColumns.Name = "btnMoveToHideColumns";
            this.btnMoveToHideColumns.Size = new System.Drawing.Size(75, 23);
            this.btnMoveToHideColumns.TabIndex = 48;
            this.btnMoveToHideColumns.Text = "<<";
            this.btnMoveToHideColumns.UseVisualStyleBackColor = true;
            this.btnMoveToHideColumns.Click += new System.EventHandler(this.BtnMoveToHideColumnsClick);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(336, 20);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(80, 13);
            this.label1.TabIndex = 47;
            this.label1.Text = "Show Columns:";
            // 
            // hiddenColumnsLabel
            // 
            this.hiddenColumnsLabel.AutoSize = true;
            this.hiddenColumnsLabel.Location = new System.Drawing.Point(23, 20);
            this.hiddenColumnsLabel.Name = "hiddenColumnsLabel";
            this.hiddenColumnsLabel.Size = new System.Drawing.Size(75, 13);
            this.hiddenColumnsLabel.TabIndex = 46;
            this.hiddenColumnsLabel.Text = "Hide Columns:";
            // 
            // showColumnsListBox
            // 
            this.showColumnsListBox.FormattingEnabled = true;
            this.showColumnsListBox.Location = new System.Drawing.Point(339, 45);
            this.showColumnsListBox.Name = "showColumnsListBox";
            this.showColumnsListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
            this.showColumnsListBox.Size = new System.Drawing.Size(205, 121);
            this.showColumnsListBox.TabIndex = 1;
            this.showColumnsListBox.SelectedIndexChanged += new System.EventHandler(this.ShowColumnsListBoxSelectedIndexChanged);
            this.showColumnsListBox.KeyUp += new System.Windows.Forms.KeyEventHandler(this.ShowColumnsListBoxKeyUp);
            // 
            // hiddenColumnsListBox
            // 
            this.hiddenColumnsListBox.FormattingEnabled = true;
            this.hiddenColumnsListBox.Location = new System.Drawing.Point(26, 45);
            this.hiddenColumnsListBox.Name = "hiddenColumnsListBox";
            this.hiddenColumnsListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
            this.hiddenColumnsListBox.Size = new System.Drawing.Size(205, 121);
            this.hiddenColumnsListBox.TabIndex = 0;
            this.hiddenColumnsListBox.SelectedIndexChanged += new System.EventHandler(this.HiddenColumnsListBoxSelectedIndexChanged);
            this.hiddenColumnsListBox.KeyUp += new System.Windows.Forms.KeyEventHandler(this.HiddenColumnsListBoxKeyUp);
            // 
            // ViewResultsPickColumnsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.pickColumnsMainPanel);
            this.Name = "ViewResultsPickColumnsControl";
            this.Size = new System.Drawing.Size(1000, 210);
            this.pickColumnsMainPanel.ResumeLayout(false);
            this.pickColumnsMainPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel pickColumnsMainPanel;
        private System.Windows.Forms.ListBox showColumnsListBox;
        private System.Windows.Forms.ListBox hiddenColumnsListBox;
        private System.Windows.Forms.Label hiddenColumnsLabel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button btnMoveDown;
        private System.Windows.Forms.Button btnMoveUp;
        private System.Windows.Forms.Button btnMoveToShowColumns;
        private System.Windows.Forms.Button btnMoveToHideColumns;
        private System.Windows.Forms.Button btnUpdateResults;
    }
}
