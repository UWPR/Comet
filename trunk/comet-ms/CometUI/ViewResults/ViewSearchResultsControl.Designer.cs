using CometUI.SharedUI;

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
            this.otherActionsTabPage = new System.Windows.Forms.TabPage();
            this.resultsPanel = new System.Windows.Forms.Panel();
            this.resultsSubPanel = new System.Windows.Forms.Panel();
            this.resultsSubPanelSplitContainer = new System.Windows.Forms.SplitContainer();
            this.resultsListView = new BrightIdeasSoftware.ObjectListView();
            this.resultsListContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.resultsListContextMenuItemExport = new System.Windows.Forms.ToolStripMenuItem();
            this.detailsPanel = new System.Windows.Forms.Panel();
            this.hideDetailsPanelButton = new System.Windows.Forms.Button();
            this.viewSpectraSplitContainer = new System.Windows.Forms.SplitContainer();
            this.graphOptionsPanel = new System.Windows.Forms.Panel();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.peakLabelRadioButtonsPanel = new System.Windows.Forms.Panel();
            this.peakLabelIonRadioButton = new System.Windows.Forms.RadioButton();
            this.peakLabelMzRadioButton = new System.Windows.Forms.RadioButton();
            this.peakLabelNoneRadioButton = new System.Windows.Forms.RadioButton();
            this.massTypeRadioButtonsPanel = new System.Windows.Forms.Panel();
            this.massTypeAvgRadioButton = new System.Windows.Forms.RadioButton();
            this.massTypeMonoRadioButton = new System.Windows.Forms.RadioButton();
            this.massTolTextBox = new CometUI.SharedUI.NumericTextBox();
            this.label26 = new System.Windows.Forms.Label();
            this.labelMassType = new System.Windows.Forms.Label();
            this.updateBtn = new System.Windows.Forms.Button();
            this.label31 = new System.Windows.Forms.Label();
            this.label32 = new System.Windows.Forms.Label();
            this.neutralLossH2OCheckBox = new System.Windows.Forms.CheckBox();
            this.labelPeakLabel = new System.Windows.Forms.Label();
            this.label30 = new System.Windows.Forms.Label();
            this.label29 = new System.Windows.Forms.Label();
            this.labelIons = new System.Windows.Forms.Label();
            this.aIonSinglyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.neutralLossNH3CheckBox = new System.Windows.Forms.CheckBox();
            this.labelNeutralLoss = new System.Windows.Forms.Label();
            this.bIonSinglyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.cIonSinglyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label25 = new System.Windows.Forms.Label();
            this.xIonSinglyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label24 = new System.Windows.Forms.Label();
            this.yIonSinglyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label23 = new System.Windows.Forms.Label();
            this.zIonSinglyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label22 = new System.Windows.Forms.Label();
            this.aIonDoublyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label21 = new System.Windows.Forms.Label();
            this.bIonDoublyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label20 = new System.Windows.Forms.Label();
            this.cIonDoublyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label19 = new System.Windows.Forms.Label();
            this.xIonDoublyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label18 = new System.Windows.Forms.Label();
            this.yIonDoublyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label17 = new System.Windows.Forms.Label();
            this.zIonDoublyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label16 = new System.Windows.Forms.Label();
            this.aIonTriplyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label15 = new System.Windows.Forms.Label();
            this.bIonTriplyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label14 = new System.Windows.Forms.Label();
            this.cIonTriplyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label13 = new System.Windows.Forms.Label();
            this.xIonTriplyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label12 = new System.Windows.Forms.Label();
            this.yIonTriplyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label11 = new System.Windows.Forms.Label();
            this.zIonTriplyChargedCheckBox = new System.Windows.Forms.CheckBox();
            this.label10 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label33 = new System.Windows.Forms.Label();
            this.spectrumGraphSplitContainer = new System.Windows.Forms.SplitContainer();
            this.precursorGraphSplitContainer = new System.Windows.Forms.SplitContainer();
            this.spectrumGraphItem = new ZedGraph.ZedGraphControl();
            this.precursorGraphItem = new ZedGraph.ZedGraphControl();
            this.spectrumGraphIonsTable = new BrightIdeasSoftware.ObjectListView();
            this.databaseLabel = new System.Windows.Forms.Label();
            this.proteinSequenceTextBox = new System.Windows.Forms.RichTextBox();
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
            ((System.ComponentModel.ISupportInitialize)(this.resultsSubPanelSplitContainer)).BeginInit();
            this.resultsSubPanelSplitContainer.Panel1.SuspendLayout();
            this.resultsSubPanelSplitContainer.Panel2.SuspendLayout();
            this.resultsSubPanelSplitContainer.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.resultsListView)).BeginInit();
            this.resultsListContextMenuStrip.SuspendLayout();
            this.detailsPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.viewSpectraSplitContainer)).BeginInit();
            this.viewSpectraSplitContainer.Panel1.SuspendLayout();
            this.viewSpectraSplitContainer.Panel2.SuspendLayout();
            this.viewSpectraSplitContainer.SuspendLayout();
            this.graphOptionsPanel.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.peakLabelRadioButtonsPanel.SuspendLayout();
            this.massTypeRadioButtonsPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.spectrumGraphSplitContainer)).BeginInit();
            this.spectrumGraphSplitContainer.Panel1.SuspendLayout();
            this.spectrumGraphSplitContainer.Panel2.SuspendLayout();
            this.spectrumGraphSplitContainer.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.precursorGraphSplitContainer)).BeginInit();
            this.precursorGraphSplitContainer.Panel1.SuspendLayout();
            this.precursorGraphSplitContainer.Panel2.SuspendLayout();
            this.precursorGraphSplitContainer.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.spectrumGraphIonsTable)).BeginInit();
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
            this.viewOptionsTab.Controls.Add(this.otherActionsTabPage);
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
            // otherActionsTabPage
            // 
            this.otherActionsTabPage.Location = new System.Drawing.Point(4, 22);
            this.otherActionsTabPage.Name = "otherActionsTabPage";
            this.otherActionsTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.otherActionsTabPage.Size = new System.Drawing.Size(1011, 197);
            this.otherActionsTabPage.TabIndex = 3;
            this.otherActionsTabPage.Text = "Other Actions";
            this.otherActionsTabPage.UseVisualStyleBackColor = true;
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
            this.resultsSubPanel.Controls.Add(this.resultsSubPanelSplitContainer);
            this.resultsSubPanel.Location = new System.Drawing.Point(18, 4);
            this.resultsSubPanel.Name = "resultsSubPanel";
            this.resultsSubPanel.Size = new System.Drawing.Size(1019, 396);
            this.resultsSubPanel.TabIndex = 2;
            // 
            // resultsSubPanelSplitContainer
            // 
            this.resultsSubPanelSplitContainer.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.resultsSubPanelSplitContainer.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.resultsSubPanelSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.resultsSubPanelSplitContainer.Name = "resultsSubPanelSplitContainer";
            this.resultsSubPanelSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // resultsSubPanelSplitContainer.Panel1
            // 
            this.resultsSubPanelSplitContainer.Panel1.Controls.Add(this.resultsListView);
            // 
            // resultsSubPanelSplitContainer.Panel2
            // 
            this.resultsSubPanelSplitContainer.Panel2.Controls.Add(this.detailsPanel);
            this.resultsSubPanelSplitContainer.Size = new System.Drawing.Size(1019, 396);
            this.resultsSubPanelSplitContainer.SplitterDistance = 198;
            this.resultsSubPanelSplitContainer.TabIndex = 4;
            // 
            // resultsListView
            // 
            this.resultsListView.AlternateRowBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(222)))), ((int)(((byte)(254)))));
            this.resultsListView.BackColor = System.Drawing.Color.White;
            this.resultsListView.ContextMenuStrip = this.resultsListContextMenuStrip;
            this.resultsListView.Cursor = System.Windows.Forms.Cursors.Default;
            this.resultsListView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.resultsListView.IncludeColumnHeadersInCopy = true;
            this.resultsListView.Location = new System.Drawing.Point(0, 0);
            this.resultsListView.Name = "resultsListView";
            this.resultsListView.ShowGroups = false;
            this.resultsListView.Size = new System.Drawing.Size(1015, 194);
            this.resultsListView.TabIndex = 1;
            this.resultsListView.UseAlternatingBackColors = true;
            this.resultsListView.UseCompatibleStateImageBehavior = false;
            this.resultsListView.UseHyperlinks = true;
            this.resultsListView.View = System.Windows.Forms.View.Details;
            this.resultsListView.CellToolTipShowing += new System.EventHandler<BrightIdeasSoftware.ToolTipShowingEventArgs>(this.ResultsListViewCellToolTipShowing);
            this.resultsListView.HyperlinkClicked += new System.EventHandler<BrightIdeasSoftware.HyperlinkClickedEventArgs>(this.ResultsListViewHyperlinkClicked);
            // 
            // resultsListContextMenuStrip
            // 
            this.resultsListContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.resultsListContextMenuItemExport});
            this.resultsListContextMenuStrip.Name = "resultsListContextMenuStrip";
            this.resultsListContextMenuStrip.Size = new System.Drawing.Size(148, 26);
            // 
            // resultsListContextMenuItemExport
            // 
            this.resultsListContextMenuItemExport.Name = "resultsListContextMenuItemExport";
            this.resultsListContextMenuItemExport.Size = new System.Drawing.Size(147, 22);
            this.resultsListContextMenuItemExport.Text = "&Export Results";
            this.resultsListContextMenuItemExport.Click += new System.EventHandler(this.ResultsListContextMenuItemExportClick);
            // 
            // detailsPanel
            // 
            this.detailsPanel.Controls.Add(this.hideDetailsPanelButton);
            this.detailsPanel.Controls.Add(this.viewSpectraSplitContainer);
            this.detailsPanel.Controls.Add(this.databaseLabel);
            this.detailsPanel.Controls.Add(this.proteinSequenceTextBox);
            this.detailsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.detailsPanel.Location = new System.Drawing.Point(0, 0);
            this.detailsPanel.Name = "detailsPanel";
            this.detailsPanel.Size = new System.Drawing.Size(1015, 190);
            this.detailsPanel.TabIndex = 1;
            // 
            // hideDetailsPanelButton
            // 
            this.hideDetailsPanelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.hideDetailsPanelButton.BackColor = System.Drawing.SystemColors.Control;
            this.hideDetailsPanelButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.hideDetailsPanelButton.ForeColor = System.Drawing.SystemColors.ControlText;
            this.hideDetailsPanelButton.Location = new System.Drawing.Point(993, 28);
            this.hideDetailsPanelButton.Name = "hideDetailsPanelButton";
            this.hideDetailsPanelButton.Size = new System.Drawing.Size(22, 20);
            this.hideDetailsPanelButton.TabIndex = 0;
            this.hideDetailsPanelButton.Text = "X";
            this.hideDetailsPanelButton.UseVisualStyleBackColor = false;
            this.hideDetailsPanelButton.Click += new System.EventHandler(this.HideDetailsPanelButtonClick);
            // 
            // viewSpectraSplitContainer
            // 
            this.viewSpectraSplitContainer.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.viewSpectraSplitContainer.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.viewSpectraSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.viewSpectraSplitContainer.IsSplitterFixed = true;
            this.viewSpectraSplitContainer.Location = new System.Drawing.Point(0, 48);
            this.viewSpectraSplitContainer.Name = "viewSpectraSplitContainer";
            // 
            // viewSpectraSplitContainer.Panel1
            // 
            this.viewSpectraSplitContainer.Panel1.Controls.Add(this.graphOptionsPanel);
            // 
            // viewSpectraSplitContainer.Panel2
            // 
            this.viewSpectraSplitContainer.Panel2.Controls.Add(this.spectrumGraphSplitContainer);
            this.viewSpectraSplitContainer.Size = new System.Drawing.Size(1015, 136);
            this.viewSpectraSplitContainer.SplitterDistance = 229;
            this.viewSpectraSplitContainer.TabIndex = 3;
            // 
            // graphOptionsPanel
            // 
            this.graphOptionsPanel.AutoScroll = true;
            this.graphOptionsPanel.BackColor = System.Drawing.SystemColors.Window;
            this.graphOptionsPanel.Controls.Add(this.groupBox2);
            this.graphOptionsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.graphOptionsPanel.Location = new System.Drawing.Point(0, 0);
            this.graphOptionsPanel.Name = "graphOptionsPanel";
            this.graphOptionsPanel.Size = new System.Drawing.Size(225, 132);
            this.graphOptionsPanel.TabIndex = 0;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.peakLabelRadioButtonsPanel);
            this.groupBox2.Controls.Add(this.massTypeRadioButtonsPanel);
            this.groupBox2.Controls.Add(this.massTolTextBox);
            this.groupBox2.Controls.Add(this.label26);
            this.groupBox2.Controls.Add(this.labelMassType);
            this.groupBox2.Controls.Add(this.updateBtn);
            this.groupBox2.Controls.Add(this.label31);
            this.groupBox2.Controls.Add(this.label32);
            this.groupBox2.Controls.Add(this.neutralLossH2OCheckBox);
            this.groupBox2.Controls.Add(this.labelPeakLabel);
            this.groupBox2.Controls.Add(this.label30);
            this.groupBox2.Controls.Add(this.label29);
            this.groupBox2.Controls.Add(this.labelIons);
            this.groupBox2.Controls.Add(this.aIonSinglyChargedCheckBox);
            this.groupBox2.Controls.Add(this.neutralLossNH3CheckBox);
            this.groupBox2.Controls.Add(this.labelNeutralLoss);
            this.groupBox2.Controls.Add(this.bIonSinglyChargedCheckBox);
            this.groupBox2.Controls.Add(this.cIonSinglyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label25);
            this.groupBox2.Controls.Add(this.xIonSinglyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label24);
            this.groupBox2.Controls.Add(this.yIonSinglyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label23);
            this.groupBox2.Controls.Add(this.zIonSinglyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label22);
            this.groupBox2.Controls.Add(this.aIonDoublyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label21);
            this.groupBox2.Controls.Add(this.bIonDoublyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label20);
            this.groupBox2.Controls.Add(this.cIonDoublyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label19);
            this.groupBox2.Controls.Add(this.xIonDoublyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label18);
            this.groupBox2.Controls.Add(this.yIonDoublyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label17);
            this.groupBox2.Controls.Add(this.zIonDoublyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label16);
            this.groupBox2.Controls.Add(this.aIonTriplyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label15);
            this.groupBox2.Controls.Add(this.bIonTriplyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label14);
            this.groupBox2.Controls.Add(this.cIonTriplyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label13);
            this.groupBox2.Controls.Add(this.xIonTriplyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label12);
            this.groupBox2.Controls.Add(this.yIonTriplyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label11);
            this.groupBox2.Controls.Add(this.zIonTriplyChargedCheckBox);
            this.groupBox2.Controls.Add(this.label10);
            this.groupBox2.Controls.Add(this.label2);
            this.groupBox2.Controls.Add(this.label9);
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Controls.Add(this.label8);
            this.groupBox2.Controls.Add(this.label4);
            this.groupBox2.Controls.Add(this.label7);
            this.groupBox2.Controls.Add(this.label5);
            this.groupBox2.Controls.Add(this.label6);
            this.groupBox2.Controls.Add(this.label33);
            this.groupBox2.Location = new System.Drawing.Point(7, -2);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(194, 390);
            this.groupBox2.TabIndex = 53;
            this.groupBox2.TabStop = false;
            // 
            // peakLabelRadioButtonsPanel
            // 
            this.peakLabelRadioButtonsPanel.Controls.Add(this.peakLabelIonRadioButton);
            this.peakLabelRadioButtonsPanel.Controls.Add(this.peakLabelMzRadioButton);
            this.peakLabelRadioButtonsPanel.Controls.Add(this.peakLabelNoneRadioButton);
            this.peakLabelRadioButtonsPanel.Location = new System.Drawing.Point(6, 226);
            this.peakLabelRadioButtonsPanel.Name = "peakLabelRadioButtonsPanel";
            this.peakLabelRadioButtonsPanel.Size = new System.Drawing.Size(182, 24);
            this.peakLabelRadioButtonsPanel.TabIndex = 73;
            // 
            // peakLabelIonRadioButton
            // 
            this.peakLabelIonRadioButton.AutoSize = true;
            this.peakLabelIonRadioButton.Location = new System.Drawing.Point(12, 2);
            this.peakLabelIonRadioButton.Name = "peakLabelIonRadioButton";
            this.peakLabelIonRadioButton.Size = new System.Drawing.Size(40, 17);
            this.peakLabelIonRadioButton.TabIndex = 67;
            this.peakLabelIonRadioButton.Text = "Ion";
            this.peakLabelIonRadioButton.UseVisualStyleBackColor = true;
            this.peakLabelIonRadioButton.CheckedChanged += new System.EventHandler(this.PeakLabelIonRadioButtonCheckedChanged);
            // 
            // peakLabelMzRadioButton
            // 
            this.peakLabelMzRadioButton.AutoSize = true;
            this.peakLabelMzRadioButton.Location = new System.Drawing.Point(75, 2);
            this.peakLabelMzRadioButton.Name = "peakLabelMzRadioButton";
            this.peakLabelMzRadioButton.Size = new System.Drawing.Size(43, 17);
            this.peakLabelMzRadioButton.TabIndex = 68;
            this.peakLabelMzRadioButton.Text = "m/z";
            this.peakLabelMzRadioButton.UseVisualStyleBackColor = true;
            this.peakLabelMzRadioButton.CheckedChanged += new System.EventHandler(this.PeakLabelMzRadioButtonCheckedChanged);
            // 
            // peakLabelNoneRadioButton
            // 
            this.peakLabelNoneRadioButton.AutoSize = true;
            this.peakLabelNoneRadioButton.Location = new System.Drawing.Point(125, 2);
            this.peakLabelNoneRadioButton.Name = "peakLabelNoneRadioButton";
            this.peakLabelNoneRadioButton.Size = new System.Drawing.Size(51, 17);
            this.peakLabelNoneRadioButton.TabIndex = 69;
            this.peakLabelNoneRadioButton.Text = "None";
            this.peakLabelNoneRadioButton.UseVisualStyleBackColor = true;
            this.peakLabelNoneRadioButton.CheckedChanged += new System.EventHandler(this.PeakLabelNoneRadioButtonCheckedChanged);
            // 
            // massTypeRadioButtonsPanel
            // 
            this.massTypeRadioButtonsPanel.Controls.Add(this.massTypeAvgRadioButton);
            this.massTypeRadioButtonsPanel.Controls.Add(this.massTypeMonoRadioButton);
            this.massTypeRadioButtonsPanel.Location = new System.Drawing.Point(6, 270);
            this.massTypeRadioButtonsPanel.Name = "massTypeRadioButtonsPanel";
            this.massTypeRadioButtonsPanel.Size = new System.Drawing.Size(182, 24);
            this.massTypeRadioButtonsPanel.TabIndex = 72;
            // 
            // massTypeAvgRadioButton
            // 
            this.massTypeAvgRadioButton.AutoSize = true;
            this.massTypeAvgRadioButton.Location = new System.Drawing.Point(75, 2);
            this.massTypeAvgRadioButton.Name = "massTypeAvgRadioButton";
            this.massTypeAvgRadioButton.Size = new System.Drawing.Size(44, 17);
            this.massTypeAvgRadioButton.TabIndex = 71;
            this.massTypeAvgRadioButton.TabStop = true;
            this.massTypeAvgRadioButton.Text = "Avg";
            this.massTypeAvgRadioButton.UseVisualStyleBackColor = true;
            this.massTypeAvgRadioButton.CheckedChanged += new System.EventHandler(this.MassTypeAvgRadioButtonCheckedChanged);
            // 
            // massTypeMonoRadioButton
            // 
            this.massTypeMonoRadioButton.AutoSize = true;
            this.massTypeMonoRadioButton.Location = new System.Drawing.Point(12, 2);
            this.massTypeMonoRadioButton.Name = "massTypeMonoRadioButton";
            this.massTypeMonoRadioButton.Size = new System.Drawing.Size(52, 17);
            this.massTypeMonoRadioButton.TabIndex = 70;
            this.massTypeMonoRadioButton.TabStop = true;
            this.massTypeMonoRadioButton.Text = "Mono";
            this.massTypeMonoRadioButton.UseVisualStyleBackColor = true;
            this.massTypeMonoRadioButton.CheckedChanged += new System.EventHandler(this.MassTypeMonoRadioButtonCheckedChanged);
            // 
            // massTolTextBox
            // 
            this.massTolTextBox.AllowDecimal = true;
            this.massTolTextBox.AllowGroupSeparator = false;
            this.massTolTextBox.AllowNegative = false;
            this.massTolTextBox.AllowSpace = false;
            this.massTolTextBox.Location = new System.Drawing.Point(18, 317);
            this.massTolTextBox.Name = "massTolTextBox";
            this.massTolTextBox.Size = new System.Drawing.Size(69, 20);
            this.massTolTextBox.TabIndex = 43;
            // 
            // label26
            // 
            this.label26.AutoSize = true;
            this.label26.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold);
            this.label26.Location = new System.Drawing.Point(3, 301);
            this.label26.Name = "label26";
            this.label26.Size = new System.Drawing.Size(62, 13);
            this.label26.TabIndex = 44;
            this.label26.Text = "Mass Tol:";
            // 
            // labelMassType
            // 
            this.labelMassType.AutoSize = true;
            this.labelMassType.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold);
            this.labelMassType.Location = new System.Drawing.Point(3, 256);
            this.labelMassType.Name = "labelMassType";
            this.labelMassType.Size = new System.Drawing.Size(72, 13);
            this.labelMassType.TabIndex = 45;
            this.labelMassType.Text = "Mass Type:";
            // 
            // updateBtn
            // 
            this.updateBtn.Location = new System.Drawing.Point(6, 356);
            this.updateBtn.Name = "updateBtn";
            this.updateBtn.Size = new System.Drawing.Size(59, 24);
            this.updateBtn.TabIndex = 48;
            this.updateBtn.Text = "&Update";
            this.updateBtn.UseVisualStyleBackColor = true;
            this.updateBtn.Click += new System.EventHandler(this.UpdateBtnClick);
            // 
            // label31
            // 
            this.label31.AutoSize = true;
            this.label31.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.5F);
            this.label31.Location = new System.Drawing.Point(107, 188);
            this.label31.Name = "label31";
            this.label31.Size = new System.Drawing.Size(10, 12);
            this.label31.TabIndex = 56;
            this.label31.Text = "2";
            // 
            // label32
            // 
            this.label32.AutoSize = true;
            this.label32.Location = new System.Drawing.Point(97, 186);
            this.label32.Name = "label32";
            this.label32.Size = new System.Drawing.Size(15, 13);
            this.label32.TabIndex = 55;
            this.label32.Text = "H";
            // 
            // neutralLossH2OCheckBox
            // 
            this.neutralLossH2OCheckBox.AutoSize = true;
            this.neutralLossH2OCheckBox.Location = new System.Drawing.Point(81, 185);
            this.neutralLossH2OCheckBox.Name = "neutralLossH2OCheckBox";
            this.neutralLossH2OCheckBox.Size = new System.Drawing.Size(15, 14);
            this.neutralLossH2OCheckBox.TabIndex = 54;
            this.neutralLossH2OCheckBox.UseVisualStyleBackColor = true;
            this.neutralLossH2OCheckBox.CheckedChanged += new System.EventHandler(this.NeutralLossH2OcheckBoxCheckedChanged);
            // 
            // labelPeakLabel
            // 
            this.labelPeakLabel.AutoSize = true;
            this.labelPeakLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelPeakLabel.Location = new System.Drawing.Point(3, 212);
            this.labelPeakLabel.Name = "labelPeakLabel";
            this.labelPeakLabel.Size = new System.Drawing.Size(75, 13);
            this.labelPeakLabel.TabIndex = 58;
            this.labelPeakLabel.Text = "Peak Label:";
            // 
            // label30
            // 
            this.label30.AutoSize = true;
            this.label30.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.5F);
            this.label30.Location = new System.Drawing.Point(52, 188);
            this.label30.Name = "label30";
            this.label30.Size = new System.Drawing.Size(10, 12);
            this.label30.TabIndex = 53;
            this.label30.Text = "3";
            // 
            // label29
            // 
            this.label29.AutoSize = true;
            this.label29.Location = new System.Drawing.Point(34, 186);
            this.label29.Name = "label29";
            this.label29.Size = new System.Drawing.Size(23, 13);
            this.label29.TabIndex = 52;
            this.label29.Text = "NH";
            // 
            // labelIons
            // 
            this.labelIons.AutoSize = true;
            this.labelIons.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold);
            this.labelIons.Location = new System.Drawing.Point(3, 16);
            this.labelIons.Name = "labelIons";
            this.labelIons.Size = new System.Drawing.Size(35, 13);
            this.labelIons.TabIndex = 0;
            this.labelIons.Text = "Ions:";
            // 
            // aIonSinglyChargedCheckBox
            // 
            this.aIonSinglyChargedCheckBox.AutoSize = true;
            this.aIonSinglyChargedCheckBox.Location = new System.Drawing.Point(31, 40);
            this.aIonSinglyChargedCheckBox.Name = "aIonSinglyChargedCheckBox";
            this.aIonSinglyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.aIonSinglyChargedCheckBox.TabIndex = 1;
            this.aIonSinglyChargedCheckBox.UseVisualStyleBackColor = true;
            this.aIonSinglyChargedCheckBox.CheckedChanged += new System.EventHandler(this.AIonSinglyChargedCheckBoxCheckedChanged);
            // 
            // neutralLossNH3CheckBox
            // 
            this.neutralLossNH3CheckBox.AutoSize = true;
            this.neutralLossNH3CheckBox.Location = new System.Drawing.Point(18, 185);
            this.neutralLossNH3CheckBox.Name = "neutralLossNH3CheckBox";
            this.neutralLossNH3CheckBox.Size = new System.Drawing.Size(15, 14);
            this.neutralLossNH3CheckBox.TabIndex = 51;
            this.neutralLossNH3CheckBox.UseVisualStyleBackColor = true;
            this.neutralLossNH3CheckBox.CheckedChanged += new System.EventHandler(this.NeutralLossNh3CheckBoxCheckedChanged);
            // 
            // labelNeutralLoss
            // 
            this.labelNeutralLoss.AutoSize = true;
            this.labelNeutralLoss.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelNeutralLoss.Location = new System.Drawing.Point(3, 169);
            this.labelNeutralLoss.Name = "labelNeutralLoss";
            this.labelNeutralLoss.Size = new System.Drawing.Size(82, 13);
            this.labelNeutralLoss.TabIndex = 50;
            this.labelNeutralLoss.Text = "Neutral Loss:";
            // 
            // bIonSinglyChargedCheckBox
            // 
            this.bIonSinglyChargedCheckBox.AutoSize = true;
            this.bIonSinglyChargedCheckBox.Location = new System.Drawing.Point(31, 60);
            this.bIonSinglyChargedCheckBox.Name = "bIonSinglyChargedCheckBox";
            this.bIonSinglyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.bIonSinglyChargedCheckBox.TabIndex = 2;
            this.bIonSinglyChargedCheckBox.UseVisualStyleBackColor = true;
            this.bIonSinglyChargedCheckBox.CheckedChanged += new System.EventHandler(this.BIonSinglyChargedCheckBoxCheckedChanged);
            // 
            // cIonSinglyChargedCheckBox
            // 
            this.cIonSinglyChargedCheckBox.AutoSize = true;
            this.cIonSinglyChargedCheckBox.Location = new System.Drawing.Point(31, 80);
            this.cIonSinglyChargedCheckBox.Name = "cIonSinglyChargedCheckBox";
            this.cIonSinglyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.cIonSinglyChargedCheckBox.TabIndex = 3;
            this.cIonSinglyChargedCheckBox.UseVisualStyleBackColor = true;
            this.cIonSinglyChargedCheckBox.CheckedChanged += new System.EventHandler(this.CIonSinglyChargedCheckBoxCheckedChanged);
            // 
            // label25
            // 
            this.label25.AutoSize = true;
            this.label25.Location = new System.Drawing.Point(147, 140);
            this.label25.Name = "label25";
            this.label25.Size = new System.Drawing.Size(19, 13);
            this.label25.TabIndex = 42;
            this.label25.Text = "3+";
            // 
            // xIonSinglyChargedCheckBox
            // 
            this.xIonSinglyChargedCheckBox.AutoSize = true;
            this.xIonSinglyChargedCheckBox.Location = new System.Drawing.Point(31, 100);
            this.xIonSinglyChargedCheckBox.Name = "xIonSinglyChargedCheckBox";
            this.xIonSinglyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.xIonSinglyChargedCheckBox.TabIndex = 4;
            this.xIonSinglyChargedCheckBox.UseVisualStyleBackColor = true;
            this.xIonSinglyChargedCheckBox.CheckedChanged += new System.EventHandler(this.XIonSinglyChargedCheckBoxCheckedChanged);
            // 
            // label24
            // 
            this.label24.AutoSize = true;
            this.label24.Location = new System.Drawing.Point(147, 120);
            this.label24.Name = "label24";
            this.label24.Size = new System.Drawing.Size(19, 13);
            this.label24.TabIndex = 41;
            this.label24.Text = "3+";
            // 
            // yIonSinglyChargedCheckBox
            // 
            this.yIonSinglyChargedCheckBox.AutoSize = true;
            this.yIonSinglyChargedCheckBox.Location = new System.Drawing.Point(31, 120);
            this.yIonSinglyChargedCheckBox.Name = "yIonSinglyChargedCheckBox";
            this.yIonSinglyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.yIonSinglyChargedCheckBox.TabIndex = 5;
            this.yIonSinglyChargedCheckBox.UseVisualStyleBackColor = true;
            this.yIonSinglyChargedCheckBox.CheckedChanged += new System.EventHandler(this.YIonSinglyChargedCheckBoxCheckedChanged);
            // 
            // label23
            // 
            this.label23.AutoSize = true;
            this.label23.Location = new System.Drawing.Point(147, 100);
            this.label23.Name = "label23";
            this.label23.Size = new System.Drawing.Size(19, 13);
            this.label23.TabIndex = 40;
            this.label23.Text = "3+";
            // 
            // zIonSinglyChargedCheckBox
            // 
            this.zIonSinglyChargedCheckBox.AutoSize = true;
            this.zIonSinglyChargedCheckBox.Location = new System.Drawing.Point(31, 140);
            this.zIonSinglyChargedCheckBox.Name = "zIonSinglyChargedCheckBox";
            this.zIonSinglyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.zIonSinglyChargedCheckBox.TabIndex = 6;
            this.zIonSinglyChargedCheckBox.UseVisualStyleBackColor = true;
            this.zIonSinglyChargedCheckBox.CheckedChanged += new System.EventHandler(this.ZIonSinglyChargedCheckBoxCheckedChanged);
            // 
            // label22
            // 
            this.label22.AutoSize = true;
            this.label22.Location = new System.Drawing.Point(147, 80);
            this.label22.Name = "label22";
            this.label22.Size = new System.Drawing.Size(19, 13);
            this.label22.TabIndex = 39;
            this.label22.Text = "3+";
            // 
            // aIonDoublyChargedCheckBox
            // 
            this.aIonDoublyChargedCheckBox.AutoSize = true;
            this.aIonDoublyChargedCheckBox.Location = new System.Drawing.Point(81, 40);
            this.aIonDoublyChargedCheckBox.Name = "aIonDoublyChargedCheckBox";
            this.aIonDoublyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.aIonDoublyChargedCheckBox.TabIndex = 7;
            this.aIonDoublyChargedCheckBox.UseVisualStyleBackColor = true;
            this.aIonDoublyChargedCheckBox.CheckedChanged += new System.EventHandler(this.AIonDoublyChargedCheckBoxCheckedChanged);
            // 
            // label21
            // 
            this.label21.AutoSize = true;
            this.label21.Location = new System.Drawing.Point(147, 60);
            this.label21.Name = "label21";
            this.label21.Size = new System.Drawing.Size(19, 13);
            this.label21.TabIndex = 38;
            this.label21.Text = "3+";
            // 
            // bIonDoublyChargedCheckBox
            // 
            this.bIonDoublyChargedCheckBox.AutoSize = true;
            this.bIonDoublyChargedCheckBox.Location = new System.Drawing.Point(81, 60);
            this.bIonDoublyChargedCheckBox.Name = "bIonDoublyChargedCheckBox";
            this.bIonDoublyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.bIonDoublyChargedCheckBox.TabIndex = 8;
            this.bIonDoublyChargedCheckBox.UseVisualStyleBackColor = true;
            this.bIonDoublyChargedCheckBox.CheckedChanged += new System.EventHandler(this.BIonDoublyChargedCheckBoxCheckedChanged);
            // 
            // label20
            // 
            this.label20.AutoSize = true;
            this.label20.Location = new System.Drawing.Point(147, 40);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(19, 13);
            this.label20.TabIndex = 37;
            this.label20.Text = "3+";
            // 
            // cIonDoublyChargedCheckBox
            // 
            this.cIonDoublyChargedCheckBox.AutoSize = true;
            this.cIonDoublyChargedCheckBox.Location = new System.Drawing.Point(81, 80);
            this.cIonDoublyChargedCheckBox.Name = "cIonDoublyChargedCheckBox";
            this.cIonDoublyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.cIonDoublyChargedCheckBox.TabIndex = 9;
            this.cIonDoublyChargedCheckBox.UseVisualStyleBackColor = true;
            this.cIonDoublyChargedCheckBox.CheckedChanged += new System.EventHandler(this.CIonDoublyChargedCheckBoxCheckedChanged);
            // 
            // label19
            // 
            this.label19.AutoSize = true;
            this.label19.Location = new System.Drawing.Point(97, 140);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(19, 13);
            this.label19.TabIndex = 36;
            this.label19.Text = "2+";
            // 
            // xIonDoublyChargedCheckBox
            // 
            this.xIonDoublyChargedCheckBox.AutoSize = true;
            this.xIonDoublyChargedCheckBox.Location = new System.Drawing.Point(81, 100);
            this.xIonDoublyChargedCheckBox.Name = "xIonDoublyChargedCheckBox";
            this.xIonDoublyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.xIonDoublyChargedCheckBox.TabIndex = 10;
            this.xIonDoublyChargedCheckBox.UseVisualStyleBackColor = true;
            this.xIonDoublyChargedCheckBox.CheckedChanged += new System.EventHandler(this.XIonDoublyChargedCheckBoxCheckedChanged);
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Location = new System.Drawing.Point(97, 120);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(19, 13);
            this.label18.TabIndex = 35;
            this.label18.Text = "2+";
            // 
            // yIonDoublyChargedCheckBox
            // 
            this.yIonDoublyChargedCheckBox.AutoSize = true;
            this.yIonDoublyChargedCheckBox.Location = new System.Drawing.Point(81, 120);
            this.yIonDoublyChargedCheckBox.Name = "yIonDoublyChargedCheckBox";
            this.yIonDoublyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.yIonDoublyChargedCheckBox.TabIndex = 11;
            this.yIonDoublyChargedCheckBox.UseVisualStyleBackColor = true;
            this.yIonDoublyChargedCheckBox.CheckedChanged += new System.EventHandler(this.YIonDoublyChargedCheckBoxCheckedChanged);
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Location = new System.Drawing.Point(97, 100);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(19, 13);
            this.label17.TabIndex = 34;
            this.label17.Text = "2+";
            // 
            // zIonDoublyChargedCheckBox
            // 
            this.zIonDoublyChargedCheckBox.AutoSize = true;
            this.zIonDoublyChargedCheckBox.Location = new System.Drawing.Point(81, 140);
            this.zIonDoublyChargedCheckBox.Name = "zIonDoublyChargedCheckBox";
            this.zIonDoublyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.zIonDoublyChargedCheckBox.TabIndex = 12;
            this.zIonDoublyChargedCheckBox.UseVisualStyleBackColor = true;
            this.zIonDoublyChargedCheckBox.CheckedChanged += new System.EventHandler(this.ZIonDoublyChargedCheckBoxCheckedChanged);
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Location = new System.Drawing.Point(97, 80);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(19, 13);
            this.label16.TabIndex = 33;
            this.label16.Text = "2+";
            // 
            // aIonTriplyChargedCheckBox
            // 
            this.aIonTriplyChargedCheckBox.AutoSize = true;
            this.aIonTriplyChargedCheckBox.Location = new System.Drawing.Point(131, 40);
            this.aIonTriplyChargedCheckBox.Name = "aIonTriplyChargedCheckBox";
            this.aIonTriplyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.aIonTriplyChargedCheckBox.TabIndex = 13;
            this.aIonTriplyChargedCheckBox.UseVisualStyleBackColor = true;
            this.aIonTriplyChargedCheckBox.CheckedChanged += new System.EventHandler(this.AIonTriplyChargedCheckBoxCheckedChanged);
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Location = new System.Drawing.Point(97, 60);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(19, 13);
            this.label15.TabIndex = 32;
            this.label15.Text = "2+";
            // 
            // bIonTriplyChargedCheckBox
            // 
            this.bIonTriplyChargedCheckBox.AutoSize = true;
            this.bIonTriplyChargedCheckBox.Location = new System.Drawing.Point(131, 60);
            this.bIonTriplyChargedCheckBox.Name = "bIonTriplyChargedCheckBox";
            this.bIonTriplyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.bIonTriplyChargedCheckBox.TabIndex = 14;
            this.bIonTriplyChargedCheckBox.UseVisualStyleBackColor = true;
            this.bIonTriplyChargedCheckBox.CheckedChanged += new System.EventHandler(this.BIonTriplyChargedCheckBoxCheckedChanged);
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(97, 40);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(19, 13);
            this.label14.TabIndex = 31;
            this.label14.Text = "2+";
            // 
            // cIonTriplyChargedCheckBox
            // 
            this.cIonTriplyChargedCheckBox.AutoSize = true;
            this.cIonTriplyChargedCheckBox.Location = new System.Drawing.Point(131, 80);
            this.cIonTriplyChargedCheckBox.Name = "cIonTriplyChargedCheckBox";
            this.cIonTriplyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.cIonTriplyChargedCheckBox.TabIndex = 15;
            this.cIonTriplyChargedCheckBox.UseVisualStyleBackColor = true;
            this.cIonTriplyChargedCheckBox.CheckedChanged += new System.EventHandler(this.CIonTriplyChargedCheckBoxCheckedChanged);
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Location = new System.Drawing.Point(47, 140);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(19, 13);
            this.label13.TabIndex = 30;
            this.label13.Text = "1+";
            // 
            // xIonTriplyChargedCheckBox
            // 
            this.xIonTriplyChargedCheckBox.AutoSize = true;
            this.xIonTriplyChargedCheckBox.Location = new System.Drawing.Point(131, 100);
            this.xIonTriplyChargedCheckBox.Name = "xIonTriplyChargedCheckBox";
            this.xIonTriplyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.xIonTriplyChargedCheckBox.TabIndex = 16;
            this.xIonTriplyChargedCheckBox.UseVisualStyleBackColor = true;
            this.xIonTriplyChargedCheckBox.CheckedChanged += new System.EventHandler(this.XIonTriplyChargedCheckBoxCheckedChanged);
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(47, 120);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(19, 13);
            this.label12.TabIndex = 29;
            this.label12.Text = "1+";
            // 
            // yIonTriplyChargedCheckBox
            // 
            this.yIonTriplyChargedCheckBox.AutoSize = true;
            this.yIonTriplyChargedCheckBox.Location = new System.Drawing.Point(131, 120);
            this.yIonTriplyChargedCheckBox.Name = "yIonTriplyChargedCheckBox";
            this.yIonTriplyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.yIonTriplyChargedCheckBox.TabIndex = 17;
            this.yIonTriplyChargedCheckBox.UseVisualStyleBackColor = true;
            this.yIonTriplyChargedCheckBox.CheckedChanged += new System.EventHandler(this.YIonTriplyChargedCheckBoxCheckedChanged);
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(47, 100);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(19, 13);
            this.label11.TabIndex = 28;
            this.label11.Text = "1+";
            // 
            // zIonTriplyChargedCheckBox
            // 
            this.zIonTriplyChargedCheckBox.AutoSize = true;
            this.zIonTriplyChargedCheckBox.Location = new System.Drawing.Point(131, 140);
            this.zIonTriplyChargedCheckBox.Name = "zIonTriplyChargedCheckBox";
            this.zIonTriplyChargedCheckBox.Size = new System.Drawing.Size(15, 14);
            this.zIonTriplyChargedCheckBox.TabIndex = 18;
            this.zIonTriplyChargedCheckBox.UseVisualStyleBackColor = true;
            this.zIonTriplyChargedCheckBox.CheckedChanged += new System.EventHandler(this.ZIonTriplyChargedCheckBoxCheckedChanged);
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(47, 80);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(19, 13);
            this.label10.TabIndex = 27;
            this.label10.Text = "1+";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(14, 39);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(14, 13);
            this.label2.TabIndex = 19;
            this.label2.Text = "a";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(47, 60);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(19, 13);
            this.label9.TabIndex = 26;
            this.label9.Text = "1+";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.Location = new System.Drawing.Point(14, 59);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(14, 13);
            this.label3.TabIndex = 20;
            this.label3.Text = "b";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(47, 40);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(19, 13);
            this.label8.TabIndex = 25;
            this.label8.Text = "1+";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.Location = new System.Drawing.Point(14, 79);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(14, 13);
            this.label4.TabIndex = 21;
            this.label4.Text = "c";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label7.Location = new System.Drawing.Point(15, 139);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(13, 13);
            this.label7.TabIndex = 24;
            this.label7.Text = "z";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.Location = new System.Drawing.Point(15, 99);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(13, 13);
            this.label5.TabIndex = 22;
            this.label5.Text = "x";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.Location = new System.Drawing.Point(15, 119);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(13, 13);
            this.label6.TabIndex = 23;
            this.label6.Text = "y";
            // 
            // label33
            // 
            this.label33.AutoSize = true;
            this.label33.Location = new System.Drawing.Point(114, 186);
            this.label33.Name = "label33";
            this.label33.Size = new System.Drawing.Size(15, 13);
            this.label33.TabIndex = 57;
            this.label33.Text = "O";
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
            this.spectrumGraphSplitContainer.Panel1.Controls.Add(this.precursorGraphSplitContainer);
            // 
            // spectrumGraphSplitContainer.Panel2
            // 
            this.spectrumGraphSplitContainer.Panel2.Controls.Add(this.spectrumGraphIonsTable);
            this.spectrumGraphSplitContainer.Size = new System.Drawing.Size(782, 136);
            this.spectrumGraphSplitContainer.SplitterDistance = 586;
            this.spectrumGraphSplitContainer.TabIndex = 0;
            // 
            // precursorGraphSplitContainer
            // 
            this.precursorGraphSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.precursorGraphSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.precursorGraphSplitContainer.Name = "precursorGraphSplitContainer";
            this.precursorGraphSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // precursorGraphSplitContainer.Panel1
            // 
            this.precursorGraphSplitContainer.Panel1.Controls.Add(this.spectrumGraphItem);
            // 
            // precursorGraphSplitContainer.Panel2
            // 
            this.precursorGraphSplitContainer.Panel2.Controls.Add(this.precursorGraphItem);
            this.precursorGraphSplitContainer.Size = new System.Drawing.Size(582, 132);
            this.precursorGraphSplitContainer.SplitterDistance = 69;
            this.precursorGraphSplitContainer.TabIndex = 2;
            // 
            // spectrumGraphItem
            // 
            this.spectrumGraphItem.Dock = System.Windows.Forms.DockStyle.Fill;
            this.spectrumGraphItem.IsEnableHPan = false;
            this.spectrumGraphItem.IsEnableVPan = false;
            this.spectrumGraphItem.Location = new System.Drawing.Point(0, 0);
            this.spectrumGraphItem.Name = "spectrumGraphItem";
            this.spectrumGraphItem.ScrollGrace = 0D;
            this.spectrumGraphItem.ScrollMaxX = 0D;
            this.spectrumGraphItem.ScrollMaxY = 0D;
            this.spectrumGraphItem.ScrollMaxY2 = 0D;
            this.spectrumGraphItem.ScrollMinX = 0D;
            this.spectrumGraphItem.ScrollMinY = 0D;
            this.spectrumGraphItem.ScrollMinY2 = 0D;
            this.spectrumGraphItem.Size = new System.Drawing.Size(582, 69);
            this.spectrumGraphItem.TabIndex = 0;
            this.spectrumGraphItem.ZoomEvent += new ZedGraph.ZedGraphControl.ZoomEventHandler(this.SpectrumGraphItemZoomEvent);
            // 
            // precursorGraphItem
            // 
            this.precursorGraphItem.Dock = System.Windows.Forms.DockStyle.Fill;
            this.precursorGraphItem.IsEnableHPan = false;
            this.precursorGraphItem.IsEnableVPan = false;
            this.precursorGraphItem.Location = new System.Drawing.Point(0, 0);
            this.precursorGraphItem.Name = "precursorGraphItem";
            this.precursorGraphItem.ScrollGrace = 0D;
            this.precursorGraphItem.ScrollMaxX = 0D;
            this.precursorGraphItem.ScrollMaxY = 0D;
            this.precursorGraphItem.ScrollMaxY2 = 0D;
            this.precursorGraphItem.ScrollMinX = 0D;
            this.precursorGraphItem.ScrollMinY = 0D;
            this.precursorGraphItem.ScrollMinY2 = 0D;
            this.precursorGraphItem.Size = new System.Drawing.Size(582, 59);
            this.precursorGraphItem.TabIndex = 1;
            this.precursorGraphItem.ZoomEvent += new ZedGraph.ZedGraphControl.ZoomEventHandler(this.PrecursorGraphItemZoomEvent);
            // 
            // spectrumGraphIonsTable
            // 
            this.spectrumGraphIonsTable.AlternateRowBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(222)))), ((int)(((byte)(254)))));
            this.spectrumGraphIonsTable.BackColor = System.Drawing.Color.White;
            this.spectrumGraphIonsTable.Cursor = System.Windows.Forms.Cursors.Default;
            this.spectrumGraphIonsTable.Dock = System.Windows.Forms.DockStyle.Fill;
            this.spectrumGraphIonsTable.HasCollapsibleGroups = false;
            this.spectrumGraphIonsTable.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.spectrumGraphIonsTable.IncludeColumnHeadersInCopy = true;
            this.spectrumGraphIonsTable.Location = new System.Drawing.Point(0, 0);
            this.spectrumGraphIonsTable.Name = "spectrumGraphIonsTable";
            this.spectrumGraphIonsTable.ShowGroups = false;
            this.spectrumGraphIonsTable.ShowSortIndicators = false;
            this.spectrumGraphIonsTable.Size = new System.Drawing.Size(188, 132);
            this.spectrumGraphIonsTable.TabIndex = 2;
            this.spectrumGraphIonsTable.UseAlternatingBackColors = true;
            this.spectrumGraphIonsTable.UseCompatibleStateImageBehavior = false;
            this.spectrumGraphIonsTable.UseHyperlinks = true;
            this.spectrumGraphIonsTable.View = System.Windows.Forms.View.Details;
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
            this.proteinSequenceTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.proteinSequenceTextBox.Location = new System.Drawing.Point(0, 48);
            this.proteinSequenceTextBox.Name = "proteinSequenceTextBox";
            this.proteinSequenceTextBox.ReadOnly = true;
            this.proteinSequenceTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
            this.proteinSequenceTextBox.Size = new System.Drawing.Size(1015, 146);
            this.proteinSequenceTextBox.TabIndex = 2;
            this.proteinSequenceTextBox.Text = "";
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
            this.resultsSubPanelSplitContainer.Panel1.ResumeLayout(false);
            this.resultsSubPanelSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.resultsSubPanelSplitContainer)).EndInit();
            this.resultsSubPanelSplitContainer.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.resultsListView)).EndInit();
            this.resultsListContextMenuStrip.ResumeLayout(false);
            this.detailsPanel.ResumeLayout(false);
            this.detailsPanel.PerformLayout();
            this.viewSpectraSplitContainer.Panel1.ResumeLayout(false);
            this.viewSpectraSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.viewSpectraSplitContainer)).EndInit();
            this.viewSpectraSplitContainer.ResumeLayout(false);
            this.graphOptionsPanel.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.peakLabelRadioButtonsPanel.ResumeLayout(false);
            this.peakLabelRadioButtonsPanel.PerformLayout();
            this.massTypeRadioButtonsPanel.ResumeLayout(false);
            this.massTypeRadioButtonsPanel.PerformLayout();
            this.spectrumGraphSplitContainer.Panel1.ResumeLayout(false);
            this.spectrumGraphSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.spectrumGraphSplitContainer)).EndInit();
            this.spectrumGraphSplitContainer.ResumeLayout(false);
            this.precursorGraphSplitContainer.Panel1.ResumeLayout(false);
            this.precursorGraphSplitContainer.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.precursorGraphSplitContainer)).EndInit();
            this.precursorGraphSplitContainer.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.spectrumGraphIonsTable)).EndInit();
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
        private System.Windows.Forms.Panel graphOptionsPanel;
        private System.Windows.Forms.Label labelIons;
        private System.Windows.Forms.CheckBox yIonSinglyChargedCheckBox;
        private System.Windows.Forms.CheckBox xIonSinglyChargedCheckBox;
        private System.Windows.Forms.CheckBox cIonSinglyChargedCheckBox;
        private System.Windows.Forms.CheckBox bIonSinglyChargedCheckBox;
        private System.Windows.Forms.CheckBox aIonSinglyChargedCheckBox;
        private System.Windows.Forms.CheckBox zIonDoublyChargedCheckBox;
        private System.Windows.Forms.CheckBox yIonDoublyChargedCheckBox;
        private System.Windows.Forms.CheckBox xIonDoublyChargedCheckBox;
        private System.Windows.Forms.CheckBox cIonDoublyChargedCheckBox;
        private System.Windows.Forms.CheckBox bIonDoublyChargedCheckBox;
        private System.Windows.Forms.CheckBox aIonDoublyChargedCheckBox;
        private System.Windows.Forms.CheckBox zIonSinglyChargedCheckBox;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.CheckBox zIonTriplyChargedCheckBox;
        private System.Windows.Forms.CheckBox yIonTriplyChargedCheckBox;
        private System.Windows.Forms.CheckBox xIonTriplyChargedCheckBox;
        private System.Windows.Forms.CheckBox cIonTriplyChargedCheckBox;
        private System.Windows.Forms.CheckBox bIonTriplyChargedCheckBox;
        private System.Windows.Forms.CheckBox aIonTriplyChargedCheckBox;
        private System.Windows.Forms.Label label25;
        private System.Windows.Forms.Label label24;
        private System.Windows.Forms.Label label23;
        private System.Windows.Forms.Label label22;
        private System.Windows.Forms.Label label21;
        private System.Windows.Forms.Label label20;
        private System.Windows.Forms.Label label19;
        private System.Windows.Forms.Label label18;
        private System.Windows.Forms.Label label17;
        private System.Windows.Forms.Label label16;
        private System.Windows.Forms.Label label15;
        private System.Windows.Forms.Label label14;
        private NumericTextBox massTolTextBox;
        private System.Windows.Forms.Label label26;
        private System.Windows.Forms.Label labelMassType;
        private System.Windows.Forms.CheckBox neutralLossNH3CheckBox;
        private System.Windows.Forms.Label labelNeutralLoss;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label label31;
        private System.Windows.Forms.Label label32;
        private System.Windows.Forms.CheckBox neutralLossH2OCheckBox;
        private System.Windows.Forms.Label label30;
        private System.Windows.Forms.Label label29;
        private System.Windows.Forms.Label label33;
        private System.Windows.Forms.Label labelPeakLabel;
        private System.Windows.Forms.Button updateBtn;
        private System.Windows.Forms.RadioButton peakLabelIonRadioButton;
        private System.Windows.Forms.RadioButton peakLabelMzRadioButton;
        private System.Windows.Forms.RadioButton peakLabelNoneRadioButton;
        private System.Windows.Forms.RadioButton massTypeAvgRadioButton;
        private System.Windows.Forms.RadioButton massTypeMonoRadioButton;
        private System.Windows.Forms.Panel massTypeRadioButtonsPanel;
        private System.Windows.Forms.Panel peakLabelRadioButtonsPanel;
        private BrightIdeasSoftware.ObjectListView spectrumGraphIonsTable;
        private System.Windows.Forms.SplitContainer resultsSubPanelSplitContainer;
        private System.Windows.Forms.SplitContainer precursorGraphSplitContainer;
        private ZedGraph.ZedGraphControl precursorGraphItem;
        private System.Windows.Forms.ContextMenuStrip resultsListContextMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem resultsListContextMenuItemExport;
        private System.Windows.Forms.TabPage otherActionsTabPage;
    }
}
