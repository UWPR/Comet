namespace CometUI.ViewResults
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
            this.components = new System.ComponentModel.Container();
            this.showOptionsPanel = new System.Windows.Forms.Panel();
            this.viewOptionsTab = new System.Windows.Forms.TabControl();
            this.summaryTabPage = new System.Windows.Forms.TabPage();
            this.displayOptionsTabPage = new System.Windows.Forms.TabPage();
            this.pickColumnsTabPage = new System.Windows.Forms.TabPage();
            this.resultsListPanel = new System.Windows.Forms.Panel();
            this.resultsListView = new System.Windows.Forms.ListView();
            this.hideOptionsGroupBox = new System.Windows.Forms.GroupBox();
            this.showHideOptionsLabel = new System.Windows.Forms.Label();
            this.showHideOptionsBtn = new System.Windows.Forms.Button();
            this.resultsListPanelFull = new System.Windows.Forms.Panel();
            this.resultsListPanelNormal = new System.Windows.Forms.Panel();
            this.viewResultsToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.showOptionsPanel.SuspendLayout();
            this.viewOptionsTab.SuspendLayout();
            this.resultsListPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // showOptionsPanel
            // 
            this.showOptionsPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.showOptionsPanel.Controls.Add(this.viewOptionsTab);
            this.showOptionsPanel.Location = new System.Drawing.Point(7, 52);
            this.showOptionsPanel.Name = "showOptionsPanel";
            this.showOptionsPanel.Size = new System.Drawing.Size(710, 270);
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
            this.viewOptionsTab.Location = new System.Drawing.Point(18, 3);
            this.viewOptionsTab.Name = "viewOptionsTab";
            this.viewOptionsTab.SelectedIndex = 0;
            this.viewOptionsTab.Size = new System.Drawing.Size(669, 262);
            this.viewOptionsTab.TabIndex = 0;
            // 
            // summaryTabPage
            // 
            this.summaryTabPage.Location = new System.Drawing.Point(4, 22);
            this.summaryTabPage.Name = "summaryTabPage";
            this.summaryTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.summaryTabPage.Size = new System.Drawing.Size(661, 236);
            this.summaryTabPage.TabIndex = 0;
            this.summaryTabPage.Text = "Summary";
            this.summaryTabPage.UseVisualStyleBackColor = true;
            // 
            // displayOptionsTabPage
            // 
            this.displayOptionsTabPage.Location = new System.Drawing.Point(4, 22);
            this.displayOptionsTabPage.Name = "displayOptionsTabPage";
            this.displayOptionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.displayOptionsTabPage.Size = new System.Drawing.Size(661, 236);
            this.displayOptionsTabPage.TabIndex = 1;
            this.displayOptionsTabPage.Text = "Display Options";
            this.displayOptionsTabPage.UseVisualStyleBackColor = true;
            // 
            // pickColumnsTabPage
            // 
            this.pickColumnsTabPage.Location = new System.Drawing.Point(4, 22);
            this.pickColumnsTabPage.Name = "pickColumnsTabPage";
            this.pickColumnsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.pickColumnsTabPage.Size = new System.Drawing.Size(661, 236);
            this.pickColumnsTabPage.TabIndex = 2;
            this.pickColumnsTabPage.Text = "Pick Columns";
            this.pickColumnsTabPage.UseVisualStyleBackColor = true;
            // 
            // resultsListPanel
            // 
            this.resultsListPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListPanel.Controls.Add(this.resultsListView);
            this.resultsListPanel.Location = new System.Drawing.Point(7, 339);
            this.resultsListPanel.Name = "resultsListPanel";
            this.resultsListPanel.Size = new System.Drawing.Size(710, 383);
            this.resultsListPanel.TabIndex = 16;
            // 
            // resultsListView
            // 
            this.resultsListView.Activation = System.Windows.Forms.ItemActivation.OneClick;
            this.resultsListView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListView.FullRowSelect = true;
            this.resultsListView.GridLines = true;
            this.resultsListView.Location = new System.Drawing.Point(18, 7);
            this.resultsListView.Name = "resultsListView";
            this.resultsListView.Size = new System.Drawing.Size(669, 336);
            this.resultsListView.TabIndex = 9;
            this.resultsListView.UseCompatibleStateImageBehavior = false;
            this.resultsListView.View = System.Windows.Forms.View.Details;
            this.resultsListView.ItemMouseHover += new System.Windows.Forms.ListViewItemMouseHoverEventHandler(this.ResultsListViewItemMouseHover);
            // 
            // hideOptionsGroupBox
            // 
            this.hideOptionsGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.hideOptionsGroupBox.Location = new System.Drawing.Point(126, 22);
            this.hideOptionsGroupBox.Name = "hideOptionsGroupBox";
            this.hideOptionsGroupBox.Size = new System.Drawing.Size(568, 5);
            this.hideOptionsGroupBox.TabIndex = 11;
            this.hideOptionsGroupBox.TabStop = false;
            // 
            // showHideOptionsLabel
            // 
            this.showHideOptionsLabel.AutoSize = true;
            this.showHideOptionsLabel.Location = new System.Drawing.Point(51, 17);
            this.showHideOptionsLabel.Name = "showHideOptionsLabel";
            this.showHideOptionsLabel.Size = new System.Drawing.Size(69, 13);
            this.showHideOptionsLabel.TabIndex = 14;
            this.showHideOptionsLabel.Text = "Hide options ";
            // 
            // showHideOptionsBtn
            // 
            this.showHideOptionsBtn.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.showHideOptionsBtn.Location = new System.Drawing.Point(25, 13);
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
            this.resultsListPanelFull.Location = new System.Drawing.Point(7, 52);
            this.resultsListPanelFull.Name = "resultsListPanelFull";
            this.resultsListPanelFull.Size = new System.Drawing.Size(710, 670);
            this.resultsListPanelFull.TabIndex = 13;
            // 
            // resultsListPanelNormal
            // 
            this.resultsListPanelNormal.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsListPanelNormal.Location = new System.Drawing.Point(7, 339);
            this.resultsListPanelNormal.Name = "resultsListPanelNormal";
            this.resultsListPanelNormal.Size = new System.Drawing.Size(710, 380);
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
            this.Size = new System.Drawing.Size(725, 725);
            this.showOptionsPanel.ResumeLayout(false);
            this.viewOptionsTab.ResumeLayout(false);
            this.resultsListPanel.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Panel showOptionsPanel;
        private System.Windows.Forms.TabControl viewOptionsTab;
        private System.Windows.Forms.TabPage summaryTabPage;
        private System.Windows.Forms.TabPage displayOptionsTabPage;
        private System.Windows.Forms.TabPage pickColumnsTabPage;
        private System.Windows.Forms.Panel resultsListPanel;
        private System.Windows.Forms.ListView resultsListView;
        private System.Windows.Forms.GroupBox hideOptionsGroupBox;
        private System.Windows.Forms.Label showHideOptionsLabel;
        private System.Windows.Forms.Button showHideOptionsBtn;
        private System.Windows.Forms.Panel resultsListPanelFull;
        private System.Windows.Forms.Panel resultsListPanelNormal;
        private System.Windows.Forms.ToolTip viewResultsToolTip;
    }
}
