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
            this.resultsPanel = new System.Windows.Forms.Panel();
            this.resultsSubPanel = new System.Windows.Forms.Panel();
            this.resultsListView = new BrightIdeasSoftware.ObjectListView();
            this.detailsPanel = new System.Windows.Forms.Panel();
            this.viewSpectraSplitContainer = new System.Windows.Forms.SplitContainer();
            this.spectrumGraphSplitContainer = new System.Windows.Forms.SplitContainer();
            this.spectrumGraphItem = new ZedGraph.ZedGraphControl();
            this.viewSpectraIonsList = new BrightIdeasSoftware.ObjectListView();
            this.databaseLabel = new System.Windows.Forms.Label();
            this.proteinSequenceTextBox = new System.Windows.Forms.RichTextBox();
            this.hideDetailsPanelButton = new System.Windows.Forms.Button();
            this.hideOptionsGroupBox = new System.Windows.Forms.GroupBox();
            this.showHideOptionsLabel = new System.Windows.Forms.Label();
            this.showHideOptionsBtn = new System.Windows.Forms.Button();
            this.resultsPanelFull = new System.Windows.Forms.Panel();
            this.resultsPanelNormal = new System.Windows.Forms.Panel();
            this.viewResultsToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.showOptionsPanel.SuspendLayout();
            this.viewOptionsTab.SuspendLayout();
            this.resultsPanel.SuspendLayout();
            this.resultsSubPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.resultsListView)).BeginInit();
            this.detailsPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.viewSpectraSplitContainer)).BeginInit();
            this.viewSpectraSplitContainer.Panel2.SuspendLayout();
            this.viewSpectraSplitContainer.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.spectrumGraphSplitContainer)).BeginInit();
            this.spectrumGraphSplitContainer.Panel1.SuspendLayout();
            this.spectrumGraphSplitContainer.Panel2.SuspendLayout();
            this.spectrumGraphSplitContainer.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.viewSpectraIonsList)).BeginInit();
            this.SuspendLayout();
            // 
            // showOptionsPanel
            // 
            this.showOptionsPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.showOptionsPanel.Controls.Add(this.viewOptionsTab);
            this.showOptionsPanel.Location = new System.Drawing.Point(7, 52);
            this.showOptionsPanel.Name = "showOptionsPanel";
            this.showOptionsPanel.Size = new System.Drawing.Size(1060, 231);
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
            this.viewOptionsTab.Size = new System.Drawing.Size(1019, 223);
            this.viewOptionsTab.TabIndex = 0;
            // 
            // summaryTabPage
            // 
            this.summaryTabPage.Location = new System.Drawing.Point(4, 22);
            this.summaryTabPage.Name = "summaryTabPage";
            this.summaryTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.summaryTabPage.Size = new System.Drawing.Size(1011, 197);
            this.summaryTabPage.TabIndex = 0;
            this.summaryTabPage.Text = "Summary";
            this.summaryTabPage.UseVisualStyleBackColor = true;
            // 
            // displayOptionsTabPage
            // 
            this.displayOptionsTabPage.Location = new System.Drawing.Point(4, 22);
            this.displayOptionsTabPage.Name = "displayOptionsTabPage";
            this.displayOptionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.displayOptionsTabPage.Size = new System.Drawing.Size(1011, 197);
            this.displayOptionsTabPage.TabIndex = 1;
            this.displayOptionsTabPage.Text = "Display Options";
            this.displayOptionsTabPage.UseVisualStyleBackColor = true;
            // 
            // pickColumnsTabPage
            // 
            this.pickColumnsTabPage.Location = new System.Drawing.Point(4, 22);
            this.pickColumnsTabPage.Name = "pickColumnsTabPage";
            this.pickColumnsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.pickColumnsTabPage.Size = new System.Drawing.Size(1011, 197);
            this.pickColumnsTabPage.TabIndex = 2;
            this.pickColumnsTabPage.Text = "Pick Columns";
            this.pickColumnsTabPage.UseVisualStyleBackColor = true;
            // 
            // resultsPanel
            // 
            this.resultsPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsPanel.Controls.Add(this.resultsSubPanel);
            this.resultsPanel.Location = new System.Drawing.Point(7, 299);
            this.resultsPanel.Name = "resultsPanel";
            this.resultsPanel.Size = new System.Drawing.Size(1060, 423);
            this.resultsPanel.TabIndex = 16;
            // 
            // resultsSubPanel
            // 
            this.resultsSubPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsSubPanel.Controls.Add(this.resultsListView);
            this.resultsSubPanel.Controls.Add(this.detailsPanel);
            this.resultsSubPanel.Location = new System.Drawing.Point(18, 4);
            this.resultsSubPanel.Name = "resultsSubPanel";
            this.resultsSubPanel.Size = new System.Drawing.Size(1019, 396);
            this.resultsSubPanel.TabIndex = 2;
            // 
            // resultsListView
            // 
            this.resultsListView.AlternateRowBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(222)))), ((int)(((byte)(254)))));
            this.resultsListView.BackColor = System.Drawing.Color.White;
            this.resultsListView.Cursor = System.Windows.Forms.Cursors.Default;
            this.resultsListView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.resultsListView.IncludeColumnHeadersInCopy = true;
            this.resultsListView.Location = new System.Drawing.Point(0, 0);
            this.resultsListView.Name = "resultsListView";
            this.resultsListView.ShowGroups = false;
            this.resultsListView.Size = new System.Drawing.Size(1019, 210);
            this.resultsListView.TabIndex = 1;
            this.resultsListView.UseAlternatingBackColors = true;
            this.resultsListView.UseCompatibleStateImageBehavior = false;
            this.resultsListView.UseHyperlinks = true;
            this.resultsListView.View = System.Windows.Forms.View.Details;
            this.resultsListView.CellToolTipShowing += new System.EventHandler<BrightIdeasSoftware.ToolTipShowingEventArgs>(this.ResultsListViewCellToolTipShowing);
            this.resultsListView.HyperlinkClicked += new System.EventHandler<BrightIdeasSoftware.HyperlinkClickedEventArgs>(this.ResultsListViewHyperlinkClicked);
            // 
            // detailsPanel
            // 
            this.detailsPanel.Controls.Add(this.viewSpectraSplitContainer);
            this.detailsPanel.Controls.Add(this.databaseLabel);
            this.detailsPanel.Controls.Add(this.proteinSequenceTextBox);
            this.detailsPanel.Controls.Add(this.hideDetailsPanelButton);
            this.detailsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.detailsPanel.Location = new System.Drawing.Point(0, 210);
            this.detailsPanel.Name = "detailsPanel";
            this.detailsPanel.Size = new System.Drawing.Size(1019, 186);
            this.detailsPanel.TabIndex = 1;
            // 
            // viewSpectraSplitContainer
            // 
            this.viewSpectraSplitContainer.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.viewSpectraSplitContainer.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.viewSpectraSplitContainer.Location = new System.Drawing.Point(0, 48);
            this.viewSpectraSplitContainer.Name = "viewSpectraSplitContainer";
            // 
            // viewSpectraSplitContainer.Panel2
            // 
            this.viewSpectraSplitContainer.Panel2.Controls.Add(this.spectrumGraphSplitContainer);
            this.viewSpectraSplitContainer.Size = new System.Drawing.Size(1019, 138);
            this.viewSpectraSplitContainer.SplitterDistance = 229;
            this.viewSpectraSplitContainer.TabIndex = 3;
            // 
            // spectrumGraphSplitContainer
            // 
            this.spectrumGraphSplitContainer.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.spectrumGraphSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.spectrumGraphSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.spectrumGraphSplitContainer.Name = "spectrumGraphSplitContainer";
            // 
            // spectrumGraphSplitContainer.Panel1
            // 
            this.spectrumGraphSplitContainer.Panel1.Controls.Add(this.spectrumGraphItem);
            // 
            // spectrumGraphSplitContainer.Panel2
            // 
            this.spectrumGraphSplitContainer.Panel2.Controls.Add(this.viewSpectraIonsList);
            this.spectrumGraphSplitContainer.Size = new System.Drawing.Size(786, 138);
            this.spectrumGraphSplitContainer.SplitterDistance = 590;
            this.spectrumGraphSplitContainer.TabIndex = 0;
            // 
            // spectrumGraphItem
            // 
            this.spectrumGraphItem.Dock = System.Windows.Forms.DockStyle.Fill;
            this.spectrumGraphItem.Location = new System.Drawing.Point(0, 0);
            this.spectrumGraphItem.Name = "spectrumGraphItem";
            this.spectrumGraphItem.ScrollGrace = 0D;
            this.spectrumGraphItem.ScrollMaxX = 0D;
            this.spectrumGraphItem.ScrollMaxY = 0D;
            this.spectrumGraphItem.ScrollMaxY2 = 0D;
            this.spectrumGraphItem.ScrollMinX = 0D;
            this.spectrumGraphItem.ScrollMinY = 0D;
            this.spectrumGraphItem.ScrollMinY2 = 0D;
            this.spectrumGraphItem.Size = new System.Drawing.Size(586, 134);
            this.spectrumGraphItem.TabIndex = 0;
            // 
            // viewSpectraIonsList
            // 
            this.viewSpectraIonsList.Dock = System.Windows.Forms.DockStyle.Fill;
            this.viewSpectraIonsList.Location = new System.Drawing.Point(0, 0);
            this.viewSpectraIonsList.Name = "viewSpectraIonsList";
            this.viewSpectraIonsList.Size = new System.Drawing.Size(188, 134);
            this.viewSpectraIonsList.TabIndex = 0;
            this.viewSpectraIonsList.UseCompatibleStateImageBehavior = false;
            this.viewSpectraIonsList.View = System.Windows.Forms.View.Details;
            // 
            // databaseLabel
            // 
            this.databaseLabel.AutoSize = true;
            this.databaseLabel.Location = new System.Drawing.Point(-3, 24);
            this.databaseLabel.Name = "databaseLabel";
            this.databaseLabel.Size = new System.Drawing.Size(35, 13);
            this.databaseLabel.TabIndex = 2;
            this.databaseLabel.Text = "label1";
            // 
            // proteinSequenceTextBox
            // 
            this.proteinSequenceTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.proteinSequenceTextBox.Location = new System.Drawing.Point(0, 40);
            this.proteinSequenceTextBox.Name = "proteinSequenceTextBox";
            this.proteinSequenceTextBox.ReadOnly = true;
            this.proteinSequenceTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
            this.proteinSequenceTextBox.Size = new System.Drawing.Size(1019, 146);
            this.proteinSequenceTextBox.TabIndex = 2;
            this.proteinSequenceTextBox.Text = "";
            // 
            // hideDetailsPanelButton
            // 
            this.hideDetailsPanelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.hideDetailsPanelButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.hideDetailsPanelButton.Location = new System.Drawing.Point(998, 24);
            this.hideDetailsPanelButton.Name = "hideDetailsPanelButton";
            this.hideDetailsPanelButton.Size = new System.Drawing.Size(21, 18);
            this.hideDetailsPanelButton.TabIndex = 0;
            this.hideDetailsPanelButton.Text = "X";
            this.hideDetailsPanelButton.UseVisualStyleBackColor = true;
            this.hideDetailsPanelButton.Click += new System.EventHandler(this.HideDetailsPanelButtonClick);
            // 
            // hideOptionsGroupBox
            // 
            this.hideOptionsGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.hideOptionsGroupBox.Location = new System.Drawing.Point(126, 22);
            this.hideOptionsGroupBox.Name = "hideOptionsGroupBox";
            this.hideOptionsGroupBox.Size = new System.Drawing.Size(918, 5);
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
            // resultsPanelFull
            // 
            this.resultsPanelFull.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsPanelFull.Location = new System.Drawing.Point(7, 52);
            this.resultsPanelFull.Name = "resultsPanelFull";
            this.resultsPanelFull.Size = new System.Drawing.Size(1060, 670);
            this.resultsPanelFull.TabIndex = 13;
            // 
            // resultsPanelNormal
            // 
            this.resultsPanelNormal.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsPanelNormal.Location = new System.Drawing.Point(7, 299);
            this.resultsPanelNormal.Name = "resultsPanelNormal";
            this.resultsPanelNormal.Size = new System.Drawing.Size(1060, 420);
            this.resultsPanelNormal.TabIndex = 17;
            // 
            // viewResultsToolTip
            // 
            this.viewResultsToolTip.UseFading = false;
            // 
            // ViewSearchResultsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.showOptionsPanel);
            this.Controls.Add(this.resultsPanel);
            this.Controls.Add(this.hideOptionsGroupBox);
            this.Controls.Add(this.showHideOptionsLabel);
            this.Controls.Add(this.showHideOptionsBtn);
            this.Controls.Add(this.resultsPanelFull);
            this.Controls.Add(this.resultsPanelNormal);
            this.Name = "ViewSearchResultsControl";
            this.Size = new System.Drawing.Size(1084, 725);
            this.showOptionsPanel.ResumeLayout(false);
            this.viewOptionsTab.ResumeLayout(false);
            this.resultsPanel.ResumeLayout(false);
            this.resultsSubPanel.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.resultsListView)).EndInit();
            this.detailsPanel.ResumeLayout(false);
            this.detailsPanel.PerformLayout();
            this.viewSpectraSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.viewSpectraSplitContainer)).EndInit();
            this.viewSpectraSplitContainer.ResumeLayout(false);
            this.spectrumGraphSplitContainer.Panel1.ResumeLayout(false);
            this.spectrumGraphSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.spectrumGraphSplitContainer)).EndInit();
            this.spectrumGraphSplitContainer.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.viewSpectraIonsList)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Panel showOptionsPanel;
        private System.Windows.Forms.TabControl viewOptionsTab;
        private System.Windows.Forms.TabPage summaryTabPage;
        private System.Windows.Forms.TabPage displayOptionsTabPage;
        private System.Windows.Forms.TabPage pickColumnsTabPage;
        private System.Windows.Forms.Panel resultsPanel;
        private System.Windows.Forms.GroupBox hideOptionsGroupBox;
        private System.Windows.Forms.Label showHideOptionsLabel;
        private System.Windows.Forms.Button showHideOptionsBtn;
        private System.Windows.Forms.Panel resultsPanelFull;
        private System.Windows.Forms.Panel resultsPanelNormal;
        private System.Windows.Forms.ToolTip viewResultsToolTip;
        private BrightIdeasSoftware.ObjectListView resultsListView;
        private System.Windows.Forms.Panel detailsPanel;
        private System.Windows.Forms.Panel resultsSubPanel;
        private System.Windows.Forms.Button hideDetailsPanelButton;
        private System.Windows.Forms.RichTextBox proteinSequenceTextBox;
        private System.Windows.Forms.Label databaseLabel;
        private System.Windows.Forms.SplitContainer viewSpectraSplitContainer;
        private System.Windows.Forms.SplitContainer spectrumGraphSplitContainer;
        private ZedGraph.ZedGraphControl spectrumGraphItem;
        private BrightIdeasSoftware.ObjectListView viewSpectraIonsList;
    }
}
