namespace CometUI.SettingsUI
{
    partial class SearchSettingsDlg
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SearchSettingsDlg));
            this.btnOK = new System.Windows.Forms.Button();
            this.mainSplitContainer = new System.Windows.Forms.SplitContainer();
            this.cometMainTab = new System.Windows.Forms.TabControl();
            this.inputFilesTabPage = new System.Windows.Forms.TabPage();
            this.outputTabPage = new System.Windows.Forms.TabPage();
            this.enzymeTabPage = new System.Windows.Forms.TabPage();
            this.massesTabPage = new System.Windows.Forms.TabPage();
            this.staticModsTabPage = new System.Windows.Forms.TabPage();
            this.varModsTabPage = new System.Windows.Forms.TabPage();
            this.miscTabPage = new System.Windows.Forms.TabPage();
            this.btnCancel = new System.Windows.Forms.Button();
            this.searchSettingsToolTip = new System.Windows.Forms.ToolTip(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).BeginInit();
            this.mainSplitContainer.Panel1.SuspendLayout();
            this.mainSplitContainer.Panel2.SuspendLayout();
            this.mainSplitContainer.SuspendLayout();
            this.cometMainTab.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOK.Location = new System.Drawing.Point(385, 16);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(78, 23);
            this.btnOK.TabIndex = 18;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.BtnOKClick);
            // 
            // mainSplitContainer
            // 
            this.mainSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mainSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
            this.mainSplitContainer.IsSplitterFixed = true;
            this.mainSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.mainSplitContainer.Name = "mainSplitContainer";
            this.mainSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // mainSplitContainer.Panel1
            // 
            this.mainSplitContainer.Panel1.Controls.Add(this.cometMainTab);
            // 
            // mainSplitContainer.Panel2
            // 
            this.mainSplitContainer.Panel2.Controls.Add(this.btnCancel);
            this.mainSplitContainer.Panel2.Controls.Add(this.btnOK);
            this.mainSplitContainer.Size = new System.Drawing.Size(559, 512);
            this.mainSplitContainer.SplitterDistance = 457;
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
            this.cometMainTab.Controls.Add(this.staticModsTabPage);
            this.cometMainTab.Controls.Add(this.varModsTabPage);
            this.cometMainTab.Controls.Add(this.miscTabPage);
            this.cometMainTab.Location = new System.Drawing.Point(12, 0);
            this.cometMainTab.Name = "cometMainTab";
            this.cometMainTab.SelectedIndex = 0;
            this.cometMainTab.Size = new System.Drawing.Size(535, 455);
            this.cometMainTab.TabIndex = 10;
            // 
            // inputFilesTabPage
            // 
            this.inputFilesTabPage.Location = new System.Drawing.Point(4, 22);
            this.inputFilesTabPage.Name = "inputFilesTabPage";
            this.inputFilesTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.inputFilesTabPage.Size = new System.Drawing.Size(527, 429);
            this.inputFilesTabPage.TabIndex = 0;
            this.inputFilesTabPage.Text = "Database";
            this.inputFilesTabPage.UseVisualStyleBackColor = true;
            // 
            // outputTabPage
            // 
            this.outputTabPage.Location = new System.Drawing.Point(4, 22);
            this.outputTabPage.Name = "outputTabPage";
            this.outputTabPage.Size = new System.Drawing.Size(527, 429);
            this.outputTabPage.TabIndex = 6;
            this.outputTabPage.Text = "Output";
            this.outputTabPage.UseVisualStyleBackColor = true;
            // 
            // enzymeTabPage
            // 
            this.enzymeTabPage.Location = new System.Drawing.Point(4, 22);
            this.enzymeTabPage.Name = "enzymeTabPage";
            this.enzymeTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.enzymeTabPage.Size = new System.Drawing.Size(527, 429);
            this.enzymeTabPage.TabIndex = 1;
            this.enzymeTabPage.Text = "Enzyme";
            this.enzymeTabPage.UseVisualStyleBackColor = true;
            // 
            // massesTabPage
            // 
            this.massesTabPage.Location = new System.Drawing.Point(4, 22);
            this.massesTabPage.Name = "massesTabPage";
            this.massesTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.massesTabPage.Size = new System.Drawing.Size(527, 429);
            this.massesTabPage.TabIndex = 2;
            this.massesTabPage.Text = "Masses";
            this.massesTabPage.UseVisualStyleBackColor = true;
            // 
            // staticModsTabPage
            // 
            this.staticModsTabPage.Location = new System.Drawing.Point(4, 22);
            this.staticModsTabPage.Name = "staticModsTabPage";
            this.staticModsTabPage.Size = new System.Drawing.Size(527, 429);
            this.staticModsTabPage.TabIndex = 7;
            this.staticModsTabPage.Text = "Static Mods";
            this.staticModsTabPage.UseVisualStyleBackColor = true;
            // 
            // varModsTabPage
            // 
            this.varModsTabPage.Location = new System.Drawing.Point(4, 22);
            this.varModsTabPage.Name = "varModsTabPage";
            this.varModsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.varModsTabPage.Size = new System.Drawing.Size(527, 429);
            this.varModsTabPage.TabIndex = 3;
            this.varModsTabPage.Text = "Var Mods";
            this.varModsTabPage.UseVisualStyleBackColor = true;
            // 
            // miscTabPage
            // 
            this.miscTabPage.Location = new System.Drawing.Point(4, 22);
            this.miscTabPage.Name = "miscTabPage";
            this.miscTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.miscTabPage.Size = new System.Drawing.Size(527, 429);
            this.miscTabPage.TabIndex = 5;
            this.miscTabPage.Text = "Misc";
            this.miscTabPage.UseVisualStyleBackColor = true;
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.Location = new System.Drawing.Point(469, 16);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(78, 23);
            this.btnCancel.TabIndex = 19;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.BtnCancelClick);
            // 
            // SearchSettingsDlg
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(559, 512);
            this.Controls.Add(this.mainSplitContainer);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(575, 550);
            this.Name = "SearchSettingsDlg";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Search Settings";
            this.mainSplitContainer.Panel1.ResumeLayout(false);
            this.mainSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).EndInit();
            this.mainSplitContainer.ResumeLayout(false);
            this.cometMainTab.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.SplitContainer mainSplitContainer;
        private System.Windows.Forms.TabControl cometMainTab;
        private System.Windows.Forms.TabPage inputFilesTabPage;
        private System.Windows.Forms.TabPage enzymeTabPage;
        private System.Windows.Forms.TabPage massesTabPage;
        private System.Windows.Forms.TabPage varModsTabPage;
        private System.Windows.Forms.TabPage miscTabPage;
        private System.Windows.Forms.TabPage outputTabPage;
        private System.Windows.Forms.TabPage staticModsTabPage;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.ToolTip searchSettingsToolTip;
    }
}

