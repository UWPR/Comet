namespace CometUI
{
    partial class CometUIMainForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(CometUIMainForm));
            this.mainMenuStrip = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.runSearchToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.settingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveCurrentToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveSearchSettingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.viewResultsSettingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.importToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.searchSettingsToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.exportToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.searchSettingsToolStripMenuItem2 = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.searchSettingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpAboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.LeftVennScoreCol = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.LeftVennPepMass3Col = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.mainStatusStrip = new System.Windows.Forms.StatusStrip();
            this.viewSearchResultsPanel = new System.Windows.Forms.Panel();
            this.mainMenuStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // mainMenuStrip
            // 
            this.mainMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.toolsToolStripMenuItem,
            this.settingsToolStripMenuItem,
            this.helpToolStripMenuItem,
            this.toolStripMenuItem1});
            this.mainMenuStrip.Location = new System.Drawing.Point(0, 0);
            this.mainMenuStrip.Name = "mainMenuStrip";
            this.mainMenuStrip.Size = new System.Drawing.Size(1084, 24);
            this.mainMenuStrip.TabIndex = 0;
            this.mainMenuStrip.Text = "mainMenuStrip";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripSeparator2,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(149, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
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
            this.saveSearchSettingsToolStripMenuItem,
            this.viewResultsSettingsToolStripMenuItem});
            this.saveCurrentToolStripMenuItem.Name = "saveCurrentToolStripMenuItem";
            this.saveCurrentToolStripMenuItem.Size = new System.Drawing.Size(163, 22);
            this.saveCurrentToolStripMenuItem.Text = "&Save Current";
            // 
            // saveSearchSettingsToolStripMenuItem
            // 
            this.saveSearchSettingsToolStripMenuItem.Name = "saveSearchSettingsToolStripMenuItem";
            this.saveSearchSettingsToolStripMenuItem.Size = new System.Drawing.Size(184, 22);
            this.saveSearchSettingsToolStripMenuItem.Text = "&Search Settings";
            this.saveSearchSettingsToolStripMenuItem.Click += new System.EventHandler(this.SaveSearchSettingsToolStripMenuItemClick);
            // 
            // viewResultsSettingsToolStripMenuItem
            // 
            this.viewResultsSettingsToolStripMenuItem.Name = "viewResultsSettingsToolStripMenuItem";
            this.viewResultsSettingsToolStripMenuItem.Size = new System.Drawing.Size(184, 22);
            this.viewResultsSettingsToolStripMenuItem.Text = "&View Results Settings";
            this.viewResultsSettingsToolStripMenuItem.Click += new System.EventHandler(this.ViewResultsSettingsToolStripMenuItemClick);
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
            // toolStripMenuItem1
            // 
            this.toolStripMenuItem1.Name = "toolStripMenuItem1";
            this.toolStripMenuItem1.Size = new System.Drawing.Size(12, 20);
            // 
            // LeftVennScoreCol
            // 
            this.LeftVennScoreCol.Width = 75;
            // 
            // LeftVennPepMass3Col
            // 
            this.LeftVennPepMass3Col.Width = 65;
            // 
            // mainStatusStrip
            // 
            this.mainStatusStrip.Location = new System.Drawing.Point(0, 740);
            this.mainStatusStrip.Name = "mainStatusStrip";
            this.mainStatusStrip.Size = new System.Drawing.Size(1084, 22);
            this.mainStatusStrip.TabIndex = 1;
            this.mainStatusStrip.Text = "statusStrip1";
            // 
            // viewSearchResultsPanel
            // 
            this.viewSearchResultsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.viewSearchResultsPanel.Location = new System.Drawing.Point(0, 24);
            this.viewSearchResultsPanel.Name = "viewSearchResultsPanel";
            this.viewSearchResultsPanel.Size = new System.Drawing.Size(1084, 716);
            this.viewSearchResultsPanel.TabIndex = 2;
            // 
            // CometUIMainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1084, 762);
            this.Controls.Add(this.viewSearchResultsPanel);
            this.Controls.Add(this.mainStatusStrip);
            this.Controls.Add(this.mainMenuStrip);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MainMenuStrip = this.mainMenuStrip;
            this.Name = "CometUIMainForm";
            this.Text = "Comet";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.CometUIFormClosing);
            this.mainMenuStrip.ResumeLayout(false);
            this.mainMenuStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip mainMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem toolsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem settingsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem1;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem runSearchToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveCurrentToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveSearchSettingsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem importToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem searchSettingsToolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem exportToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem searchSettingsToolStripMenuItem2;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem searchSettingsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem helpAboutToolStripMenuItem;
        private System.Windows.Forms.ColumnHeader LeftVennScoreCol;
        private System.Windows.Forms.ColumnHeader LeftVennPepMass3Col;
        private System.Windows.Forms.StatusStrip mainStatusStrip;
        private System.Windows.Forms.Panel viewSearchResultsPanel;
        private System.Windows.Forms.ToolStripMenuItem viewResultsSettingsToolStripMenuItem;
    }
}