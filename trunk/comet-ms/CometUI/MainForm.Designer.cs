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
            this.btnSearch = new System.Windows.Forms.Button();
            this.menuStripMain = new System.Windows.Forms.MenuStrip();
            this.toolStripMenuItemHelp = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mainSplitContainer = new System.Windows.Forms.SplitContainer();
            this.cometMainTab = new System.Windows.Forms.TabControl();
            this.inputFilesTabPage = new System.Windows.Forms.TabPage();
            this.enzymeTabPage = new System.Windows.Forms.TabPage();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.btnLoadParams = new System.Windows.Forms.Button();
            this.btnSaveParams = new System.Windows.Forms.Button();
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
            this.btnSearch.Click += new System.EventHandler(this.BtnTestClick);
            // 
            // menuStripMain
            // 
            this.menuStripMain.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItemHelp});
            this.menuStripMain.Location = new System.Drawing.Point(0, 0);
            this.menuStripMain.Name = "menuStripMain";
            this.menuStripMain.Size = new System.Drawing.Size(559, 24);
            this.menuStripMain.TabIndex = 1;
            this.menuStripMain.Text = "Main Menu";
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
            this.mainSplitContainer.Panel2.Controls.Add(this.btnLoadParams);
            this.mainSplitContainer.Panel2.Controls.Add(this.btnSaveParams);
            this.mainSplitContainer.Panel2.Controls.Add(this.statusStripMain);
            this.mainSplitContainer.Panel2.Controls.Add(this.btnSearch);
            this.mainSplitContainer.Size = new System.Drawing.Size(559, 413);
            this.mainSplitContainer.SplitterDistance = 358;
            this.mainSplitContainer.TabIndex = 2;
            // 
            // cometMainTab
            // 
            this.cometMainTab.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.cometMainTab.Controls.Add(this.inputFilesTabPage);
            this.cometMainTab.Controls.Add(this.enzymeTabPage);
            this.cometMainTab.Controls.Add(this.tabPage1);
            this.cometMainTab.Controls.Add(this.tabPage2);
            this.cometMainTab.Controls.Add(this.tabPage3);
            this.cometMainTab.Controls.Add(this.tabPage4);
            this.cometMainTab.Location = new System.Drawing.Point(12, 0);
            this.cometMainTab.Name = "cometMainTab";
            this.cometMainTab.SelectedIndex = 0;
            this.cometMainTab.Size = new System.Drawing.Size(535, 356);
            this.cometMainTab.TabIndex = 10;
            // 
            // inputFilesTabPage
            // 
            this.inputFilesTabPage.Location = new System.Drawing.Point(4, 22);
            this.inputFilesTabPage.Name = "inputFilesTabPage";
            this.inputFilesTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.inputFilesTabPage.Size = new System.Drawing.Size(527, 330);
            this.inputFilesTabPage.TabIndex = 0;
            this.inputFilesTabPage.Text = "Input Files";
            this.inputFilesTabPage.UseVisualStyleBackColor = true;
            // 
            // enzymeTabPage
            // 
            this.enzymeTabPage.Location = new System.Drawing.Point(4, 22);
            this.enzymeTabPage.Name = "enzymeTabPage";
            this.enzymeTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.enzymeTabPage.Size = new System.Drawing.Size(527, 330);
            this.enzymeTabPage.TabIndex = 1;
            this.enzymeTabPage.Text = "Enzyme";
            this.enzymeTabPage.UseVisualStyleBackColor = true;
            // 
            // tabPage1
            // 
            this.tabPage1.Location = new System.Drawing.Point(4, 22);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(527, 330);
            this.tabPage1.TabIndex = 2;
            this.tabPage1.Text = "Masses";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // tabPage2
            // 
            this.tabPage2.Location = new System.Drawing.Point(4, 22);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(527, 330);
            this.tabPage2.TabIndex = 3;
            this.tabPage2.Text = "Mods";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // tabPage3
            // 
            this.tabPage3.Location = new System.Drawing.Point(4, 22);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage3.Size = new System.Drawing.Size(527, 330);
            this.tabPage3.TabIndex = 4;
            this.tabPage3.Text = "Output";
            this.tabPage3.UseVisualStyleBackColor = true;
            // 
            // tabPage4
            // 
            this.tabPage4.Location = new System.Drawing.Point(4, 22);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage4.Size = new System.Drawing.Size(527, 330);
            this.tabPage4.TabIndex = 5;
            this.tabPage4.Text = "Misc";
            this.tabPage4.UseVisualStyleBackColor = true;
            // 
            // btnLoadParams
            // 
            this.btnLoadParams.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnLoadParams.Location = new System.Drawing.Point(301, 0);
            this.btnLoadParams.Name = "btnLoadParams";
            this.btnLoadParams.Size = new System.Drawing.Size(78, 23);
            this.btnLoadParams.TabIndex = 16;
            this.btnLoadParams.Text = "Load Params";
            this.btnLoadParams.UseVisualStyleBackColor = true;
            // 
            // btnSaveParams
            // 
            this.btnSaveParams.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnSaveParams.Location = new System.Drawing.Point(385, 0);
            this.btnSaveParams.Name = "btnSaveParams";
            this.btnSaveParams.Size = new System.Drawing.Size(78, 23);
            this.btnSaveParams.TabIndex = 17;
            this.btnSaveParams.Text = "Save Params";
            this.btnSaveParams.UseVisualStyleBackColor = true;
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
            this.ClientSize = new System.Drawing.Size(559, 437);
            this.Controls.Add(this.mainSplitContainer);
            this.Controls.Add(this.menuStripMain);
            this.MainMenuStrip = this.menuStripMain;
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
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.TabPage tabPage3;
        private System.Windows.Forms.TabPage tabPage4;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.Button btnSaveParams;
        private System.Windows.Forms.Button btnLoadParams;
    }
}

