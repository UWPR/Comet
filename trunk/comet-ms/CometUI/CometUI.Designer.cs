namespace CometUI
{
    partial class CometUI
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(CometUI));
            this.mainMenuStrip = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.runSearchToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.settingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveCurrentToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveSearchSettingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.importToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.searchSettingsToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.exportToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.searchSettingsToolStripMenuItem2 = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.searchSettingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpAboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mainStatusStrip = new System.Windows.Forms.StatusStrip();
            this.viewOptionsTab = new System.Windows.Forms.TabControl();
            this.summaryTabPage = new System.Windows.Forms.TabPage();
            this.displayOptionsTabPage = new System.Windows.Forms.TabPage();
            this.pickColumnsTabPage = new System.Windows.Forms.TabPage();
            this.filteringOptionsTabPage = new System.Windows.Forms.TabPage();
            this.otherActionsTabPage = new System.Windows.Forms.TabPage();
            this.showHideOptionsBtn = new System.Windows.Forms.Button();
            this.showHideOptionsLabel = new System.Windows.Forms.Label();
            this.hideOptionsGroupBox = new System.Windows.Forms.GroupBox();
            this.showOptionsPanel = new System.Windows.Forms.Panel();
            this.resultsListPanel = new System.Windows.Forms.Panel();
            this.resultsListView = new System.Windows.Forms.ListView();
            this.LeftVennScoreCol = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.LeftVennSequenceCol = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.LeftVennScanNumCol = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.LeftVennPepMass2Col = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.LeftVennPepMass3Col = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.resultsListPanelNormal = new System.Windows.Forms.Panel();
            this.resultsListPanelFull = new System.Windows.Forms.Panel();
            this.mainMenuStrip.SuspendLayout();
            this.viewOptionsTab.SuspendLayout();
            this.showOptionsPanel.SuspendLayout();
            this.resultsListPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // mainMenuStrip
            // 
            this.mainMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.toolsToolStripMenuItem,
            this.settingsToolStripMenuItem,
            this.helpToolStripMenuItem});
            this.mainMenuStrip.Location = new System.Drawing.Point(0, 0);
            this.mainMenuStrip.Name = "mainMenuStrip";
            this.mainMenuStrip.Size = new System.Drawing.Size(734, 24);
            this.mainMenuStrip.TabIndex = 0;
            this.mainMenuStrip.Text = "mainMenuStrip";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openToolStripMenuItem,
            this.toolStripSeparator2,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // openToolStripMenuItem
            // 
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            this.openToolStripMenuItem.Size = new System.Drawing.Size(112, 22);
            this.openToolStripMenuItem.Text = "&Open...";
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(109, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(112, 22);
            this.exitToolStripMenuItem.Text = "&Exit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.ExitToolStripMenuItemClick);
            // 
            // toolsToolStripMenuItem
            // 
            this.toolsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.runSearchToolStripMenuItem});
            this.toolsToolStripMenuItem.Name = "toolsToolStripMenuItem";
            this.toolsToolStripMenuItem.Size = new System.Drawing.Size(48, 20);
            this.toolsToolStripMenuItem.Text = "&Tools";
            // 
            // runSearchToolStripMenuItem
            // 
            this.runSearchToolStripMenuItem.Name = "runSearchToolStripMenuItem";
            this.runSearchToolStripMenuItem.Size = new System.Drawing.Size(142, 22);
            this.runSearchToolStripMenuItem.Text = "&Run Search...";
            this.runSearchToolStripMenuItem.Click += new System.EventHandler(this.RunSearchToolStripMenuItemClick);
            // 
            // settingsToolStripMenuItem
            // 
            this.settingsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.saveCurrentToolStripMenuItem,
            this.importToolStripMenuItem,
            this.exportToolStripMenuItem,
            this.toolStripSeparator1,
            this.searchSettingsToolStripMenuItem});
            this.settingsToolStripMenuItem.Name = "settingsToolStripMenuItem";
            this.settingsToolStripMenuItem.Size = new System.Drawing.Size(61, 20);
            this.settingsToolStripMenuItem.Text = "&Settings";
            // 
            // saveCurrentToolStripMenuItem
            // 
            this.saveCurrentToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.saveSearchSettingsToolStripMenuItem});
            this.saveCurrentToolStripMenuItem.Name = "saveCurrentToolStripMenuItem";
            this.saveCurrentToolStripMenuItem.Size = new System.Drawing.Size(163, 22);
            this.saveCurrentToolStripMenuItem.Text = "&Save Current";
            // 
            // saveSearchSettingsToolStripMenuItem
            // 
            this.saveSearchSettingsToolStripMenuItem.Name = "saveSearchSettingsToolStripMenuItem";
            this.saveSearchSettingsToolStripMenuItem.Size = new System.Drawing.Size(154, 22);
            this.saveSearchSettingsToolStripMenuItem.Text = "&Search Settings";
            this.saveSearchSettingsToolStripMenuItem.Click += new System.EventHandler(this.SaveSearchSettingsToolStripMenuItemClick);
            // 
            // importToolStripMenuItem
            // 
            this.importToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.searchSettingsToolStripMenuItem1});
            this.importToolStripMenuItem.Name = "importToolStripMenuItem";
            this.importToolStripMenuItem.Size = new System.Drawing.Size(163, 22);
            this.importToolStripMenuItem.Text = "&Import";
            // 
            // searchSettingsToolStripMenuItem1
            // 
            this.searchSettingsToolStripMenuItem1.Name = "searchSettingsToolStripMenuItem1";
            this.searchSettingsToolStripMenuItem1.Size = new System.Drawing.Size(154, 22);
            this.searchSettingsToolStripMenuItem1.Text = "&Search Settings";
            this.searchSettingsToolStripMenuItem1.Click += new System.EventHandler(this.SearchSettingsImportToolStripMenuItemClick);
            // 
            // exportToolStripMenuItem
            // 
            this.exportToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.searchSettingsToolStripMenuItem2});
            this.exportToolStripMenuItem.Name = "exportToolStripMenuItem";
            this.exportToolStripMenuItem.Size = new System.Drawing.Size(163, 22);
            this.exportToolStripMenuItem.Text = "&Export";
            // 
            // searchSettingsToolStripMenuItem2
            // 
            this.searchSettingsToolStripMenuItem2.Name = "searchSettingsToolStripMenuItem2";
            this.searchSettingsToolStripMenuItem2.Size = new System.Drawing.Size(154, 22);
            this.searchSettingsToolStripMenuItem2.Text = "&Search Settings";
            this.searchSettingsToolStripMenuItem2.Click += new System.EventHandler(this.SearchSettingsExportToolStripMenuItemClick);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(160, 6);
            // 
            // searchSettingsToolStripMenuItem
            // 
            this.searchSettingsToolStripMenuItem.Name = "searchSettingsToolStripMenuItem";
            this.searchSettingsToolStripMenuItem.Size = new System.Drawing.Size(163, 22);
            this.searchSettingsToolStripMenuItem.Text = "Sea&rch Settings...";
            this.searchSettingsToolStripMenuItem.Click += new System.EventHandler(this.SearchSettingsToolStripMenuItemClick);
            // 
            // helpToolStripMenuItem
            // 
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.helpAboutToolStripMenuItem});
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            this.helpToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.helpToolStripMenuItem.Text = "&Help";
            // 
            // helpAboutToolStripMenuItem
            // 
            this.helpAboutToolStripMenuItem.Name = "helpAboutToolStripMenuItem";
            this.helpAboutToolStripMenuItem.Size = new System.Drawing.Size(146, 22);
            this.helpAboutToolStripMenuItem.Text = "&About Comet";
            this.helpAboutToolStripMenuItem.Click += new System.EventHandler(this.HelpAboutToolStripMenuItemClick);
            // 
            // mainStatusStrip
            // 
            this.mainStatusStrip.Location = new System.Drawing.Point(0, 490);
            this.mainStatusStrip.Name = "mainStatusStrip";
            this.mainStatusStrip.Size = new System.Drawing.Size(734, 22);
            this.mainStatusStrip.TabIndex = 1;
            this.mainStatusStrip.Text = "statusStrip1";
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
            this.viewOptionsTab.Size = new System.Drawing.Size(661, 172);
            this.viewOptionsTab.TabIndex = 0;
            // 
            // summaryTabPage
            // 
            this.summaryTabPage.Location = new System.Drawing.Point(4, 22);
            this.summaryTabPage.Name = "summaryTabPage";
            this.summaryTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.summaryTabPage.Size = new System.Drawing.Size(653, 146);
            this.summaryTabPage.TabIndex = 0;
            this.summaryTabPage.Text = "Summary";
            this.summaryTabPage.UseVisualStyleBackColor = true;
            // 
            // displayOptionsTabPage
            // 
            this.displayOptionsTabPage.Location = new System.Drawing.Point(4, 22);
            this.displayOptionsTabPage.Name = "displayOptionsTabPage";
            this.displayOptionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.displayOptionsTabPage.Size = new System.Drawing.Size(653, 146);
            this.displayOptionsTabPage.TabIndex = 1;
            this.displayOptionsTabPage.Text = "Display Options";
            this.displayOptionsTabPage.UseVisualStyleBackColor = true;
            // 
            // pickColumnsTabPage
            // 
            this.pickColumnsTabPage.Location = new System.Drawing.Point(4, 22);
            this.pickColumnsTabPage.Name = "pickColumnsTabPage";
            this.pickColumnsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.pickColumnsTabPage.Size = new System.Drawing.Size(653, 146);
            this.pickColumnsTabPage.TabIndex = 2;
            this.pickColumnsTabPage.Text = "Pick Columns";
            this.pickColumnsTabPage.UseVisualStyleBackColor = true;
            // 
            // filteringOptionsTabPage
            // 
            this.filteringOptionsTabPage.Location = new System.Drawing.Point(4, 22);
            this.filteringOptionsTabPage.Name = "filteringOptionsTabPage";
            this.filteringOptionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.filteringOptionsTabPage.Size = new System.Drawing.Size(653, 146);
            this.filteringOptionsTabPage.TabIndex = 3;
            this.filteringOptionsTabPage.Text = "Filtering Options";
            this.filteringOptionsTabPage.UseVisualStyleBackColor = true;
            // 
            // otherActionsTabPage
            // 
            this.otherActionsTabPage.Location = new System.Drawing.Point(4, 22);
            this.otherActionsTabPage.Name = "otherActionsTabPage";
            this.otherActionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.otherActionsTabPage.Size = new System.Drawing.Size(653, 146);
            this.otherActionsTabPage.TabIndex = 4;
            this.otherActionsTabPage.Text = "Other Actions";
            this.otherActionsTabPage.UseVisualStyleBackColor = true;
            // 
            // showHideOptionsBtn
            // 
            this.showHideOptionsBtn.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.showHideOptionsBtn.Location = new System.Drawing.Point(12, 27);
            this.showHideOptionsBtn.Name = "showHideOptionsBtn";
            this.showHideOptionsBtn.Size = new System.Drawing.Size(20, 20);
            this.showHideOptionsBtn.TabIndex = 0;
            this.showHideOptionsBtn.Text = "+";
            this.showHideOptionsBtn.UseVisualStyleBackColor = true;
            this.showHideOptionsBtn.Click += new System.EventHandler(this.ShowHideOptionsBtnClick);
            // 
            // showHideOptionsLabel
            // 
            this.showHideOptionsLabel.AutoSize = true;
            this.showHideOptionsLabel.Location = new System.Drawing.Point(35, 31);
            this.showHideOptionsLabel.Name = "showHideOptionsLabel";
            this.showHideOptionsLabel.Size = new System.Drawing.Size(69, 13);
            this.showHideOptionsLabel.TabIndex = 1;
            this.showHideOptionsLabel.Text = "Hide options ";
            // 
            // hideOptionsGroupBox
            // 
            this.hideOptionsGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.hideOptionsGroupBox.Location = new System.Drawing.Point(110, 36);
            this.hideOptionsGroupBox.Name = "hideOptionsGroupBox";
            this.hideOptionsGroupBox.Size = new System.Drawing.Size(589, 5);
            this.hideOptionsGroupBox.TabIndex = 0;
            this.hideOptionsGroupBox.TabStop = false;
            // 
            // showOptionsPanel
            // 
            this.showOptionsPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.showOptionsPanel.Controls.Add(this.viewOptionsTab);
            this.showOptionsPanel.Location = new System.Drawing.Point(12, 55);
            this.showOptionsPanel.Name = "showOptionsPanel";
            this.showOptionsPanel.Size = new System.Drawing.Size(710, 180);
            this.showOptionsPanel.TabIndex = 6;
            // 
            // resultsListPanel
            // 
            this.resultsListPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListPanel.Controls.Add(this.resultsListView);
            this.resultsListPanel.Location = new System.Drawing.Point(12, 241);
            this.resultsListPanel.Name = "resultsListPanel";
            this.resultsListPanel.Size = new System.Drawing.Size(710, 246);
            this.resultsListPanel.TabIndex = 7;
            // 
            // resultsListView
            // 
            this.resultsListView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.LeftVennScoreCol,
            this.LeftVennSequenceCol,
            this.LeftVennScanNumCol,
            this.LeftVennPepMass2Col,
            this.LeftVennPepMass3Col});
            this.resultsListView.FullRowSelect = true;
            this.resultsListView.GridLines = true;
            this.resultsListView.Location = new System.Drawing.Point(26, 9);
            this.resultsListView.Name = "resultsListView";
            this.resultsListView.Size = new System.Drawing.Size(661, 223);
            this.resultsListView.TabIndex = 9;
            this.resultsListView.UseCompatibleStateImageBehavior = false;
            this.resultsListView.View = System.Windows.Forms.View.Details;
            // 
            // LeftVennScoreCol
            // 
            this.LeftVennScoreCol.Width = 75;
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
            // LeftVennPepMass3Col
            // 
            this.LeftVennPepMass3Col.Width = 65;
            // 
            // resultsListPanelNormal
            // 
            this.resultsListPanelNormal.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListPanelNormal.Location = new System.Drawing.Point(12, 241);
            this.resultsListPanelNormal.Name = "resultsListPanelNormal";
            this.resultsListPanelNormal.Size = new System.Drawing.Size(710, 246);
            this.resultsListPanelNormal.TabIndex = 10;
            // 
            // resultsListPanelFull
            // 
            this.resultsListPanelFull.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListPanelFull.Location = new System.Drawing.Point(12, 53);
            this.resultsListPanelFull.Name = "resultsListPanelFull";
            this.resultsListPanelFull.Size = new System.Drawing.Size(710, 434);
            this.resultsListPanelFull.TabIndex = 0;
            // 
            // CometUI
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(734, 512);
            this.Controls.Add(this.showOptionsPanel);
            this.Controls.Add(this.resultsListPanel);
            this.Controls.Add(this.hideOptionsGroupBox);
            this.Controls.Add(this.showHideOptionsLabel);
            this.Controls.Add(this.showHideOptionsBtn);
            this.Controls.Add(this.mainStatusStrip);
            this.Controls.Add(this.mainMenuStrip);
            this.Controls.Add(this.resultsListPanelFull);
            this.Controls.Add(this.resultsListPanelNormal);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MainMenuStrip = this.mainMenuStrip;
            this.Name = "CometUI";
            this.Text = "Comet";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.CometUIFormClosing);
            this.mainMenuStrip.ResumeLayout(false);
            this.mainMenuStrip.PerformLayout();
            this.viewOptionsTab.ResumeLayout(false);
            this.showOptionsPanel.ResumeLayout(false);
            this.resultsListPanel.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip mainMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpAboutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem settingsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exportToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveCurrentToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem toolsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem runSearchToolStripMenuItem;
        private System.Windows.Forms.StatusStrip mainStatusStrip;
        private System.Windows.Forms.ToolStripMenuItem searchSettingsToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem importToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem searchSettingsToolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem searchSettingsToolStripMenuItem2;
        private System.Windows.Forms.ToolStripMenuItem saveSearchSettingsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.TabControl viewOptionsTab;
        private System.Windows.Forms.TabPage summaryTabPage;
        private System.Windows.Forms.TabPage displayOptionsTabPage;
        private System.Windows.Forms.TabPage pickColumnsTabPage;
        private System.Windows.Forms.TabPage filteringOptionsTabPage;
        private System.Windows.Forms.TabPage otherActionsTabPage;
        private System.Windows.Forms.Button showHideOptionsBtn;
        private System.Windows.Forms.Label showHideOptionsLabel;
        private System.Windows.Forms.GroupBox hideOptionsGroupBox;
        private System.Windows.Forms.Panel showOptionsPanel;
        private System.Windows.Forms.Panel resultsListPanel;
        private System.Windows.Forms.ListView resultsListView;
        private System.Windows.Forms.ColumnHeader LeftVennScoreCol;
        private System.Windows.Forms.ColumnHeader LeftVennSequenceCol;
        private System.Windows.Forms.ColumnHeader LeftVennScanNumCol;
        private System.Windows.Forms.ColumnHeader LeftVennPepMass2Col;
        private System.Windows.Forms.ColumnHeader LeftVennPepMass3Col;
        private System.Windows.Forms.Panel resultsListPanelNormal;
        private System.Windows.Forms.Panel resultsListPanelFull;
    }
}