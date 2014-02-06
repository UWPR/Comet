namespace CometUI
{
    partial class CometMainForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(CometMainForm));
            this.btnSearch = new System.Windows.Forms.Button();
            this.menuStripMain = new System.Windows.Forms.MenuStrip();
            this.toolStripMenuItemSettings = new System.Windows.Forms.ToolStripMenuItem();
            this.saveAsDefaultToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.importFromFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exportToFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItemHelp = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mainSplitContainer = new System.Windows.Forms.SplitContainer();
            this.cometMainTab = new System.Windows.Forms.TabControl();
            this.inputFilesTabPage = new System.Windows.Forms.TabPage();
            this.outputTabPage = new System.Windows.Forms.TabPage();
            this.enzymeTabPage = new System.Windows.Forms.TabPage();
            this.massesTabPage = new System.Windows.Forms.TabPage();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.statusStripMain = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.menuStripMain.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).BeginInit();
            this.mainSplitContainer.Panel1.SuspendLayout();
            this.mainSplitContainer.Panel2.SuspendLayout();
            this.mainSplitContainer.SuspendLayout();
            this.cometMainTab.SuspendLayout();
            this.statusStripMain.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnSearch
            // 
            this.btnSearch.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnSearch.Location = new System.Drawing.Point(469, 0);
            this.btnSearch.Name = "btnSearch";
            this.btnSearch.Size = new System.Drawing.Size(78, 23);
            this.btnSearch.TabIndex = 18;
            this.btnSearch.Text = "Search";
            this.btnSearch.UseVisualStyleBackColor = true;
            this.btnSearch.Click += new System.EventHandler(this.BtnSearchClick);
            // 
            // menuStripMain
            // 
            this.menuStripMain.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItemSettings,
            this.toolStripMenuItemHelp});
            this.menuStripMain.Location = new System.Drawing.Point(0, 0);
            this.menuStripMain.Name = "menuStripMain";
            this.menuStripMain.Size = new System.Drawing.Size(559, 24);
            this.menuStripMain.TabIndex = 1;
            this.menuStripMain.Text = "Main Menu";
            // 
            // toolStripMenuItemSettings
            // 
            this.toolStripMenuItemSettings.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.saveAsDefaultToolStripMenuItem,
            this.importFromFileToolStripMenuItem,
            this.exportToFileToolStripMenuItem});
            this.toolStripMenuItemSettings.Name = "toolStripMenuItemSettings";
            this.toolStripMenuItemSettings.Size = new System.Drawing.Size(61, 20);
            this.toolStripMenuItemSettings.Text = "&Settings";
            // 
            // saveAsDefaultToolStripMenuItem
            // 
            this.saveAsDefaultToolStripMenuItem.Name = "saveAsDefaultToolStripMenuItem";
            this.saveAsDefaultToolStripMenuItem.Size = new System.Drawing.Size(162, 22);
            this.saveAsDefaultToolStripMenuItem.Text = "&Save as Default...";
            // 
            // importFromFileToolStripMenuItem
            // 
            this.importFromFileToolStripMenuItem.Name = "importFromFileToolStripMenuItem";
            this.importFromFileToolStripMenuItem.Size = new System.Drawing.Size(162, 22);
            this.importFromFileToolStripMenuItem.Text = "&Import...";
            // 
            // exportToFileToolStripMenuItem
            // 
            this.exportToFileToolStripMenuItem.Name = "exportToFileToolStripMenuItem";
            this.exportToFileToolStripMenuItem.Size = new System.Drawing.Size(162, 22);
            this.exportToFileToolStripMenuItem.Text = "&Export...";
            // 
            // toolStripMenuItemHelp
            // 
            this.toolStripMenuItemHelp.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.aboutToolStripMenuItem});
            this.toolStripMenuItemHelp.Name = "toolStripMenuItemHelp";
            this.toolStripMenuItemHelp.Size = new System.Drawing.Size(44, 20);
            this.toolStripMenuItemHelp.Text = "&Help";
            // 
            // aboutToolStripMenuItem
            // 
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size(107, 22);
            this.aboutToolStripMenuItem.Text = "&About";
            // 
            // mainSplitContainer
            // 
            this.mainSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mainSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this.mainSplitContainer.IsSplitterFixed = true;
            this.mainSplitContainer.Location = new System.Drawing.Point(0, 24);
            this.mainSplitContainer.Name = "mainSplitContainer";
            this.mainSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // mainSplitContainer.Panel1
            // 
            this.mainSplitContainer.Panel1.Controls.Add(this.cometMainTab);
            // 
            // mainSplitContainer.Panel2
            // 
            this.mainSplitContainer.Panel2.Controls.Add(this.statusStripMain);
            this.mainSplitContainer.Panel2.Controls.Add(this.btnSearch);
            this.mainSplitContainer.Size = new System.Drawing.Size(559, 438);
            this.mainSplitContainer.SplitterDistance = 383;
            this.mainSplitContainer.TabIndex = 2;
            // 
            // cometMainTab
            // 
            this.cometMainTab.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.cometMainTab.Controls.Add(this.inputFilesTabPage);
            this.cometMainTab.Controls.Add(this.outputTabPage);
            this.cometMainTab.Controls.Add(this.enzymeTabPage);
            this.cometMainTab.Controls.Add(this.massesTabPage);
            this.cometMainTab.Controls.Add(this.tabPage2);
            this.cometMainTab.Controls.Add(this.tabPage4);
            this.cometMainTab.Location = new System.Drawing.Point(12, 0);
            this.cometMainTab.Name = "cometMainTab";
            this.cometMainTab.SelectedIndex = 0;
            this.cometMainTab.Size = new System.Drawing.Size(535, 381);
            this.cometMainTab.TabIndex = 10;
            // 
            // inputFilesTabPage
            // 
            this.inputFilesTabPage.Location = new System.Drawing.Point(4, 22);
            this.inputFilesTabPage.Name = "inputFilesTabPage";
            this.inputFilesTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.inputFilesTabPage.Size = new System.Drawing.Size(527, 355);
            this.inputFilesTabPage.TabIndex = 0;
            this.inputFilesTabPage.Text = "Input/Output";
            this.inputFilesTabPage.UseVisualStyleBackColor = true;
            // 
            // outputTabPage
            // 
            this.outputTabPage.Location = new System.Drawing.Point(4, 22);
            this.outputTabPage.Name = "outputTabPage";
            this.outputTabPage.Size = new System.Drawing.Size(527, 330);
            this.outputTabPage.TabIndex = 6;
            this.outputTabPage.Text = "Output";
            this.outputTabPage.UseVisualStyleBackColor = true;
            // 
            // enzymeTabPage
            // 
            this.enzymeTabPage.Location = new System.Drawing.Point(4, 22);
            this.enzymeTabPage.Name = "enzymeTabPage";
            this.enzymeTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.enzymeTabPage.Size = new System.Drawing.Size(527, 505);
            this.enzymeTabPage.TabIndex = 1;
            this.enzymeTabPage.Text = "Enzyme";
            this.enzymeTabPage.UseVisualStyleBackColor = true;
            // 
            // massesTabPage
            // 
            this.massesTabPage.Location = new System.Drawing.Point(4, 22);
            this.massesTabPage.Name = "massesTabPage";
            this.massesTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.massesTabPage.Size = new System.Drawing.Size(527, 505);
            this.massesTabPage.TabIndex = 2;
            this.massesTabPage.Text = "Masses";
            this.massesTabPage.UseVisualStyleBackColor = true;
            // 
            // tabPage2
            // 
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(527, 505);
            this.tabPage2.TabIndex = 3;
            this.tabPage2.Text = "Mods";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // tabPage4
            // 
            this.tabPage4.Location = new System.Drawing.Point(4, 22);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage4.Size = new System.Drawing.Size(527, 505);
            this.tabPage4.TabIndex = 5;
            this.tabPage4.Text = "Misc";
            this.tabPage4.UseVisualStyleBackColor = true;
            // 
            // statusStripMain
            // 
            this.statusStripMain.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1});
            this.statusStripMain.Location = new System.Drawing.Point(0, 29);
            this.statusStripMain.Name = "statusStripMain";
            this.statusStripMain.Size = new System.Drawing.Size(559, 22);
            this.statusStripMain.TabIndex = 19;
            this.statusStripMain.Text = "statusStrip";
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(218, 17);
            this.toolStripStatusLabel1.Text = "This is where search status will show up.";
            // 
            // CometMainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(559, 462);
            this.Controls.Add(this.mainSplitContainer);
            this.Controls.Add(this.menuStripMain);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MainMenuStrip = this.menuStripMain;
            this.MinimumSize = new System.Drawing.Size(575, 500);
            this.Name = "CometMainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Comet";
            this.menuStripMain.ResumeLayout(false);
            this.menuStripMain.PerformLayout();
            this.mainSplitContainer.Panel1.ResumeLayout(false);
            this.mainSplitContainer.Panel2.ResumeLayout(false);
            this.mainSplitContainer.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).EndInit();
            this.mainSplitContainer.ResumeLayout(false);
            this.cometMainTab.ResumeLayout(false);
            this.statusStripMain.ResumeLayout(false);
            this.statusStripMain.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnSearch;
        private System.Windows.Forms.MenuStrip menuStripMain;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItemHelp;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.SplitContainer mainSplitContainer;
        private System.Windows.Forms.TabControl cometMainTab;
        private System.Windows.Forms.TabPage inputFilesTabPage;
        private System.Windows.Forms.TabPage enzymeTabPage;
        private System.Windows.Forms.StatusStrip statusStripMain;
        private System.Windows.Forms.TabPage massesTabPage;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.TabPage tabPage4;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItemSettings;
        private System.Windows.Forms.ToolStripMenuItem saveAsDefaultToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exportToFileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem importFromFileToolStripMenuItem;
        private System.Windows.Forms.TabPage outputTabPage;
    }
}

