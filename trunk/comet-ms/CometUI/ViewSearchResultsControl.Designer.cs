namespace CometUI
{
    partial class ViewSearchResultsControl
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
            this.showOptionsPanel = new System.Windows.Forms.Panel();
            this.viewOptionsTab = new System.Windows.Forms.TabControl();
            this.summaryTabPage = new System.Windows.Forms.TabPage();
            this.displayOptionsTabPage = new System.Windows.Forms.TabPage();
            this.pickColumnsTabPage = new System.Windows.Forms.TabPage();
            this.filteringOptionsTabPage = new System.Windows.Forms.TabPage();
            this.otherActionsTabPage = new System.Windows.Forms.TabPage();
            this.resultsListPanel = new System.Windows.Forms.Panel();
            this.pageNavPanel = new System.Windows.Forms.Panel();
            this.linkLabelFirstPage = new System.Windows.Forms.LinkLabel();
            this.linkLabelLastPage = new System.Windows.Forms.LinkLabel();
            this.pageNumbersPanel = new System.Windows.Forms.Panel();
            this.linkLabelPage6 = new System.Windows.Forms.LinkLabel();
            this.linkLabelPage9 = new System.Windows.Forms.LinkLabel();
            this.linkLabelPage8 = new System.Windows.Forms.LinkLabel();
            this.linkLabelPage10 = new System.Windows.Forms.LinkLabel();
            this.linkLabelPage7 = new System.Windows.Forms.LinkLabel();
            this.linkLabelPage1 = new System.Windows.Forms.LinkLabel();
            this.linkLabelPage2 = new System.Windows.Forms.LinkLabel();
            this.linkLabelPage5 = new System.Windows.Forms.LinkLabel();
            this.linkLabelPage3 = new System.Windows.Forms.LinkLabel();
            this.linkLabelPage4 = new System.Windows.Forms.LinkLabel();
            this.linkLabelNextPage = new System.Windows.Forms.LinkLabel();
            this.linkLabelPreviousPage = new System.Windows.Forms.LinkLabel();
            this.resultsListView = new System.Windows.Forms.ListView();
            this.LeftVennSequenceCol = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.LeftVennScanNumCol = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.LeftVennPepMass2Col = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.hideOptionsGroupBox = new System.Windows.Forms.GroupBox();
            this.showHideOptionsLabel = new System.Windows.Forms.Label();
            this.showHideOptionsBtn = new System.Windows.Forms.Button();
            this.resultsListPanelFull = new System.Windows.Forms.Panel();
            this.resultsListPanelNormal = new System.Windows.Forms.Panel();
            this.showOptionsPanel.SuspendLayout();
            this.viewOptionsTab.SuspendLayout();
            this.resultsListPanel.SuspendLayout();
            this.pageNavPanel.SuspendLayout();
            this.pageNumbersPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // showOptionsPanel
            // 
            this.showOptionsPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.showOptionsPanel.Controls.Add(this.viewOptionsTab);
            this.showOptionsPanel.Location = new System.Drawing.Point(7, 45);
            this.showOptionsPanel.Name = "showOptionsPanel";
            this.showOptionsPanel.Size = new System.Drawing.Size(710, 161);
            this.showOptionsPanel.TabIndex = 15;
            // 
            // viewOptionsTab
            // 
            this.viewOptionsTab.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.viewOptionsTab.Controls.Add(this.summaryTabPage);
            this.viewOptionsTab.Controls.Add(this.displayOptionsTabPage);
            this.viewOptionsTab.Controls.Add(this.pickColumnsTabPage);
            this.viewOptionsTab.Controls.Add(this.filteringOptionsTabPage);
            this.viewOptionsTab.Controls.Add(this.otherActionsTabPage);
            this.viewOptionsTab.Location = new System.Drawing.Point(26, 3);
            this.viewOptionsTab.Name = "viewOptionsTab";
            this.viewOptionsTab.SelectedIndex = 0;
            this.viewOptionsTab.Size = new System.Drawing.Size(661, 153);
            this.viewOptionsTab.TabIndex = 0;
            // 
            // summaryTabPage
            // 
            this.summaryTabPage.Location = new System.Drawing.Point(4, 22);
            this.summaryTabPage.Name = "summaryTabPage";
            this.summaryTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.summaryTabPage.Size = new System.Drawing.Size(653, 127);
            this.summaryTabPage.TabIndex = 0;
            this.summaryTabPage.Text = "Summary";
            this.summaryTabPage.UseVisualStyleBackColor = true;
            // 
            // displayOptionsTabPage
            // 
            this.displayOptionsTabPage.Location = new System.Drawing.Point(4, 22);
            this.displayOptionsTabPage.Name = "displayOptionsTabPage";
            this.displayOptionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.displayOptionsTabPage.Size = new System.Drawing.Size(653, 127);
            this.displayOptionsTabPage.TabIndex = 1;
            this.displayOptionsTabPage.Text = "Display Options";
            this.displayOptionsTabPage.UseVisualStyleBackColor = true;
            // 
            // pickColumnsTabPage
            // 
            this.pickColumnsTabPage.Location = new System.Drawing.Point(4, 22);
            this.pickColumnsTabPage.Name = "pickColumnsTabPage";
            this.pickColumnsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.pickColumnsTabPage.Size = new System.Drawing.Size(653, 127);
            this.pickColumnsTabPage.TabIndex = 2;
            this.pickColumnsTabPage.Text = "Pick Columns";
            this.pickColumnsTabPage.UseVisualStyleBackColor = true;
            // 
            // filteringOptionsTabPage
            // 
            this.filteringOptionsTabPage.Location = new System.Drawing.Point(4, 22);
            this.filteringOptionsTabPage.Name = "filteringOptionsTabPage";
            this.filteringOptionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.filteringOptionsTabPage.Size = new System.Drawing.Size(653, 127);
            this.filteringOptionsTabPage.TabIndex = 3;
            this.filteringOptionsTabPage.Text = "Filtering Options";
            this.filteringOptionsTabPage.UseVisualStyleBackColor = true;
            // 
            // otherActionsTabPage
            // 
            this.otherActionsTabPage.Location = new System.Drawing.Point(4, 22);
            this.otherActionsTabPage.Name = "otherActionsTabPage";
            this.otherActionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.otherActionsTabPage.Size = new System.Drawing.Size(653, 127);
            this.otherActionsTabPage.TabIndex = 4;
            this.otherActionsTabPage.Text = "Other Actions";
            this.otherActionsTabPage.UseVisualStyleBackColor = true;
            // 
            // resultsListPanel
            // 
            this.resultsListPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListPanel.Controls.Add(this.pageNavPanel);
            this.resultsListPanel.Controls.Add(this.resultsListView);
            this.resultsListPanel.Location = new System.Drawing.Point(7, 231);
            this.resultsListPanel.Name = "resultsListPanel";
            this.resultsListPanel.Size = new System.Drawing.Size(710, 227);
            this.resultsListPanel.TabIndex = 16;
            // 
            // pageNavPanel
            // 
            this.pageNavPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.pageNavPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.pageNavPanel.Controls.Add(this.linkLabelFirstPage);
            this.pageNavPanel.Controls.Add(this.linkLabelLastPage);
            this.pageNavPanel.Controls.Add(this.pageNumbersPanel);
            this.pageNavPanel.Controls.Add(this.linkLabelNextPage);
            this.pageNavPanel.Controls.Add(this.linkLabelPreviousPage);
            this.pageNavPanel.Location = new System.Drawing.Point(26, 197);
            this.pageNavPanel.Name = "pageNavPanel";
            this.pageNavPanel.Size = new System.Drawing.Size(428, 20);
            this.pageNavPanel.TabIndex = 12;
            // 
            // linkLabelFirstPage
            // 
            this.linkLabelFirstPage.AutoSize = true;
            this.linkLabelFirstPage.Location = new System.Drawing.Point(3, 6);
            this.linkLabelFirstPage.Name = "linkLabelFirstPage";
            this.linkLabelFirstPage.Size = new System.Drawing.Size(26, 13);
            this.linkLabelFirstPage.TabIndex = 26;
            this.linkLabelFirstPage.TabStop = true;
            this.linkLabelFirstPage.Text = "First";
            // 
            // linkLabelLastPage
            // 
            this.linkLabelLastPage.AutoSize = true;
            this.linkLabelLastPage.Location = new System.Drawing.Point(398, 6);
            this.linkLabelLastPage.Name = "linkLabelLastPage";
            this.linkLabelLastPage.Size = new System.Drawing.Size(27, 13);
            this.linkLabelLastPage.TabIndex = 25;
            this.linkLabelLastPage.TabStop = true;
            this.linkLabelLastPage.Text = "Last";
            // 
            // pageNumbersPanel
            // 
            this.pageNumbersPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.pageNumbersPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage6);
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage9);
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage8);
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage10);
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage7);
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage1);
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage2);
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage5);
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage3);
            this.pageNumbersPanel.Controls.Add(this.linkLabelPage4);
            this.pageNumbersPanel.Location = new System.Drawing.Point(93, 5);
            this.pageNumbersPanel.Name = "pageNumbersPanel";
            this.pageNumbersPanel.Size = new System.Drawing.Size(255, 19);
            this.pageNumbersPanel.TabIndex = 13;
            // 
            // linkLabelPage6
            // 
            this.linkLabelPage6.AutoSize = true;
            this.linkLabelPage6.Location = new System.Drawing.Point(129, 1);
            this.linkLabelPage6.Name = "linkLabelPage6";
            this.linkLabelPage6.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage6.TabIndex = 20;
            this.linkLabelPage6.TabStop = true;
            this.linkLabelPage6.Text = "16";
            // 
            // linkLabelPage9
            // 
            this.linkLabelPage9.AutoSize = true;
            this.linkLabelPage9.Location = new System.Drawing.Point(204, 1);
            this.linkLabelPage9.Name = "linkLabelPage9";
            this.linkLabelPage9.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage9.TabIndex = 13;
            this.linkLabelPage9.TabStop = true;
            this.linkLabelPage9.Text = "19";
            // 
            // linkLabelPage8
            // 
            this.linkLabelPage8.AutoSize = true;
            this.linkLabelPage8.Location = new System.Drawing.Point(179, 1);
            this.linkLabelPage8.Name = "linkLabelPage8";
            this.linkLabelPage8.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage8.TabIndex = 22;
            this.linkLabelPage8.TabStop = true;
            this.linkLabelPage8.Text = "18";
            // 
            // linkLabelPage10
            // 
            this.linkLabelPage10.AutoSize = true;
            this.linkLabelPage10.Location = new System.Drawing.Point(229, 1);
            this.linkLabelPage10.Name = "linkLabelPage10";
            this.linkLabelPage10.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage10.TabIndex = 14;
            this.linkLabelPage10.TabStop = true;
            this.linkLabelPage10.Text = "20";
            // 
            // linkLabelPage7
            // 
            this.linkLabelPage7.AutoSize = true;
            this.linkLabelPage7.Location = new System.Drawing.Point(154, 1);
            this.linkLabelPage7.Name = "linkLabelPage7";
            this.linkLabelPage7.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage7.TabIndex = 21;
            this.linkLabelPage7.TabStop = true;
            this.linkLabelPage7.Text = "17";
            // 
            // linkLabelPage1
            // 
            this.linkLabelPage1.AutoSize = true;
            this.linkLabelPage1.Location = new System.Drawing.Point(4, 1);
            this.linkLabelPage1.Name = "linkLabelPage1";
            this.linkLabelPage1.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage1.TabIndex = 15;
            this.linkLabelPage1.TabStop = true;
            this.linkLabelPage1.Text = "11";
            // 
            // linkLabelPage2
            // 
            this.linkLabelPage2.AutoSize = true;
            this.linkLabelPage2.Location = new System.Drawing.Point(29, 1);
            this.linkLabelPage2.Name = "linkLabelPage2";
            this.linkLabelPage2.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage2.TabIndex = 16;
            this.linkLabelPage2.TabStop = true;
            this.linkLabelPage2.Text = "12";
            // 
            // linkLabelPage5
            // 
            this.linkLabelPage5.AutoSize = true;
            this.linkLabelPage5.Location = new System.Drawing.Point(104, 1);
            this.linkLabelPage5.Name = "linkLabelPage5";
            this.linkLabelPage5.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage5.TabIndex = 19;
            this.linkLabelPage5.TabStop = true;
            this.linkLabelPage5.Text = "15";
            // 
            // linkLabelPage3
            // 
            this.linkLabelPage3.AutoSize = true;
            this.linkLabelPage3.Location = new System.Drawing.Point(54, 1);
            this.linkLabelPage3.Name = "linkLabelPage3";
            this.linkLabelPage3.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage3.TabIndex = 17;
            this.linkLabelPage3.TabStop = true;
            this.linkLabelPage3.Text = "13";
            // 
            // linkLabelPage4
            // 
            this.linkLabelPage4.AutoSize = true;
            this.linkLabelPage4.Location = new System.Drawing.Point(79, 1);
            this.linkLabelPage4.Name = "linkLabelPage4";
            this.linkLabelPage4.Size = new System.Drawing.Size(19, 13);
            this.linkLabelPage4.TabIndex = 18;
            this.linkLabelPage4.TabStop = true;
            this.linkLabelPage4.Text = "14";
            // 
            // linkLabelNextPage
            // 
            this.linkLabelNextPage.AutoSize = true;
            this.linkLabelNextPage.Location = new System.Drawing.Point(354, 6);
            this.linkLabelNextPage.Name = "linkLabelNextPage";
            this.linkLabelNextPage.Size = new System.Drawing.Size(38, 13);
            this.linkLabelNextPage.TabIndex = 24;
            this.linkLabelNextPage.TabStop = true;
            this.linkLabelNextPage.Text = "Next >";
            // 
            // linkLabelPreviousPage
            // 
            this.linkLabelPreviousPage.AutoSize = true;
            this.linkLabelPreviousPage.Location = new System.Drawing.Point(35, 6);
            this.linkLabelPreviousPage.Name = "linkLabelPreviousPage";
            this.linkLabelPreviousPage.Size = new System.Drawing.Size(57, 13);
            this.linkLabelPreviousPage.TabIndex = 23;
            this.linkLabelPreviousPage.TabStop = true;
            this.linkLabelPreviousPage.Text = "< Previous";
            // 
            // resultsListView
            // 
            this.resultsListView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.LeftVennSequenceCol,
            this.LeftVennScanNumCol,
            this.LeftVennPepMass2Col});
            this.resultsListView.FullRowSelect = true;
            this.resultsListView.GridLines = true;
            this.resultsListView.Location = new System.Drawing.Point(26, 7);
            this.resultsListView.Name = "resultsListView";
            this.resultsListView.Size = new System.Drawing.Size(661, 180);
            this.resultsListView.TabIndex = 9;
            this.resultsListView.UseCompatibleStateImageBehavior = false;
            this.resultsListView.View = System.Windows.Forms.View.Details;
            // 
            // LeftVennSequenceCol
            // 
            this.LeftVennSequenceCol.Width = 102;
            // 
            // LeftVennScanNumCol
            // 
            this.LeftVennScanNumCol.Width = 67;
            // 
            // LeftVennPepMass2Col
            // 
            this.LeftVennPepMass2Col.Width = 68;
            // 
            // hideOptionsGroupBox
            // 
            this.hideOptionsGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.hideOptionsGroupBox.Location = new System.Drawing.Point(105, 22);
            this.hideOptionsGroupBox.Name = "hideOptionsGroupBox";
            this.hideOptionsGroupBox.Size = new System.Drawing.Size(589, 5);
            this.hideOptionsGroupBox.TabIndex = 11;
            this.hideOptionsGroupBox.TabStop = false;
            // 
            // showHideOptionsLabel
            // 
            this.showHideOptionsLabel.AutoSize = true;
            this.showHideOptionsLabel.Location = new System.Drawing.Point(30, 17);
            this.showHideOptionsLabel.Name = "showHideOptionsLabel";
            this.showHideOptionsLabel.Size = new System.Drawing.Size(69, 13);
            this.showHideOptionsLabel.TabIndex = 14;
            this.showHideOptionsLabel.Text = "Hide options ";
            // 
            // showHideOptionsBtn
            // 
            this.showHideOptionsBtn.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.showHideOptionsBtn.Location = new System.Drawing.Point(7, 13);
            this.showHideOptionsBtn.Name = "showHideOptionsBtn";
            this.showHideOptionsBtn.Size = new System.Drawing.Size(20, 20);
            this.showHideOptionsBtn.TabIndex = 12;
            this.showHideOptionsBtn.Text = "+";
            this.showHideOptionsBtn.UseVisualStyleBackColor = true;
            this.showHideOptionsBtn.Click += new System.EventHandler(this.ShowHideOptionsBtnClick);
            // 
            // resultsListPanelFull
            // 
            this.resultsListPanelFull.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListPanelFull.Location = new System.Drawing.Point(7, 43);
            this.resultsListPanelFull.Name = "resultsListPanelFull";
            this.resultsListPanelFull.Size = new System.Drawing.Size(710, 415);
            this.resultsListPanelFull.TabIndex = 13;
            // 
            // resultsListPanelNormal
            // 
            this.resultsListPanelNormal.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListPanelNormal.Location = new System.Drawing.Point(7, 215);
            this.resultsListPanelNormal.Name = "resultsListPanelNormal";
            this.resultsListPanelNormal.Size = new System.Drawing.Size(710, 246);
            this.resultsListPanelNormal.TabIndex = 17;
            // 
            // ViewSearchResultsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.showOptionsPanel);
            this.Controls.Add(this.resultsListPanel);
            this.Controls.Add(this.hideOptionsGroupBox);
            this.Controls.Add(this.showHideOptionsLabel);
            this.Controls.Add(this.showHideOptionsBtn);
            this.Controls.Add(this.resultsListPanelFull);
            this.Controls.Add(this.resultsListPanelNormal);
            this.Name = "ViewSearchResultsControl";
            this.Size = new System.Drawing.Size(725, 475);
            this.showOptionsPanel.ResumeLayout(false);
            this.viewOptionsTab.ResumeLayout(false);
            this.resultsListPanel.ResumeLayout(false);
            this.pageNavPanel.ResumeLayout(false);
            this.pageNavPanel.PerformLayout();
            this.pageNumbersPanel.ResumeLayout(false);
            this.pageNumbersPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Panel showOptionsPanel;
        private System.Windows.Forms.TabControl viewOptionsTab;
        private System.Windows.Forms.TabPage summaryTabPage;
        private System.Windows.Forms.TabPage displayOptionsTabPage;
        private System.Windows.Forms.TabPage pickColumnsTabPage;
        private System.Windows.Forms.TabPage filteringOptionsTabPage;
        private System.Windows.Forms.TabPage otherActionsTabPage;
        private System.Windows.Forms.Panel resultsListPanel;
        private System.Windows.Forms.Panel pageNavPanel;
        private System.Windows.Forms.LinkLabel linkLabelFirstPage;
        private System.Windows.Forms.LinkLabel linkLabelLastPage;
        private System.Windows.Forms.Panel pageNumbersPanel;
        private System.Windows.Forms.LinkLabel linkLabelPage6;
        private System.Windows.Forms.LinkLabel linkLabelPage9;
        private System.Windows.Forms.LinkLabel linkLabelPage8;
        private System.Windows.Forms.LinkLabel linkLabelPage10;
        private System.Windows.Forms.LinkLabel linkLabelPage7;
        private System.Windows.Forms.LinkLabel linkLabelPage1;
        private System.Windows.Forms.LinkLabel linkLabelPage2;
        private System.Windows.Forms.LinkLabel linkLabelPage5;
        private System.Windows.Forms.LinkLabel linkLabelPage3;
        private System.Windows.Forms.LinkLabel linkLabelPage4;
        private System.Windows.Forms.LinkLabel linkLabelNextPage;
        private System.Windows.Forms.LinkLabel linkLabelPreviousPage;
        private System.Windows.Forms.ListView resultsListView;
        private System.Windows.Forms.ColumnHeader LeftVennSequenceCol;
        private System.Windows.Forms.ColumnHeader LeftVennScanNumCol;
        private System.Windows.Forms.ColumnHeader LeftVennPepMass2Col;
        private System.Windows.Forms.GroupBox hideOptionsGroupBox;
        private System.Windows.Forms.Label showHideOptionsLabel;
        private System.Windows.Forms.Button showHideOptionsBtn;
        private System.Windows.Forms.Panel resultsListPanelFull;
        private System.Windows.Forms.Panel resultsListPanelNormal;
    }
}
