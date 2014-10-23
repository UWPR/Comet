using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Windows.Forms;

namespace CometUI.ViewResults
{
    public partial class ViewResultsPickColumnsControl : UserControl
    {
        private ViewSearchResultsControl ViewSearchResultsControl { get; set; }

        public ViewResultsPickColumnsControl(ViewSearchResultsControl parent)
        {
            InitializeComponent();

            ViewSearchResultsControl = parent;

            InitializeFromDefaultSettings();
        }

        private void InitializeFromDefaultSettings()
        {
            foreach (var item in CometUI.ViewResultsSettings.PickColumnsHideList)
            {
                hiddenColumnsListBox.Items.Add(item);
            }

            foreach (var item in CometUI.ViewResultsSettings.PickColumnsShowList)
            {
                showColumnsListBox.Items.Add(item);
            }
        }

        private void VerifyAndUpdateHideColumnsSetting()
        {
            var currentHiddenColumnsStringCollection = ListBoxToStringCollection(hiddenColumnsListBox);
            if (HasPickColumnsListChanged(currentHiddenColumnsStringCollection, CometUI.ViewResultsSettings.PickColumnsHideList))
            {
                CometUI.ViewResultsSettings.PickColumnsHideList = currentHiddenColumnsStringCollection;
                ViewSearchResultsControl.SettingsChanged = true;
            }
        }

        private void VerifyAndUpdateShowColumnsSetting()
        {
            var currentShowColumnsStringCollection = ListBoxToStringCollection(showColumnsListBox);
            if (HasPickColumnsListChanged(currentShowColumnsStringCollection, CometUI.ViewResultsSettings.PickColumnsShowList))
            {
                CometUI.ViewResultsSettings.PickColumnsShowList = currentShowColumnsStringCollection;
                ViewSearchResultsControl.SettingsChanged = true;
            }
        }

        private bool HasPickColumnsListChanged(StringCollection currentList, StringCollection listInSettings)
        {
            var settingsChanged = false;
            if (currentList.Count != listInSettings.Count)
            {
                settingsChanged = true;
            }
            else
            {
                for (int i = 0; i < currentList.Count; i++)
                {
                    if (!currentList[i].Equals(listInSettings[i]))
                    {
                        settingsChanged = true;
                        break;
                    }
                }
            }

            return settingsChanged;
        }

        private StringCollection ListBoxToStringCollection(ListBox listBox)
        {
            var strCollection = new StringCollection();
            foreach (var item in listBox.Items)
            {
                strCollection.Add(item.ToString());
            }

            return strCollection;
        }

        private void MoveUp(ListBox listBox)
        {
            var selectedIndices = listBox.SelectedIndices;
            if (selectedIndices.Count > 0)
            {
                var insertIndex = selectedIndices[0];
                var insertItems = new List<String>();
                if (insertIndex > 0)
                {
                    foreach (int index in selectedIndices)
                    {
                        var item = listBox.Items[index] as String;
                        insertItems.Add(item);
                    }

                    for (int i = 0; i < insertItems.Count; i++)
                    {
                        listBox.Items.Remove(insertItems[i]);
                    }

                    for (int i = 0; i < insertItems.Count; i++)
                    {
                        listBox.Items.Insert(insertIndex - 1, insertItems[insertItems.Count - i - 1]);
                    }

                    listBox.SelectedIndex = insertIndex - 1;
                }
            }
        }

        private void MoveDown(ListBox listBox)
        {
            var selectedIndices = listBox.SelectedIndices;
            if (selectedIndices.Count > 0)
            {
                int insertIndex = selectedIndices[0];
                int lastSelectedIndex = selectedIndices[selectedIndices.Count - 1];
                if (lastSelectedIndex != (listBox.Items.Count -1))
                {
                    var insertItems = new List<String>();

                    foreach (int index in selectedIndices)
                    {
                        var item = listBox.Items[index] as String;
                        insertItems.Add(item);
                    }

                    for (int i = 0; i < insertItems.Count; i++)
                    {
                        listBox.Items.Remove(insertItems[i]);
                    }

                    for (int i = 0; i < insertItems.Count; i++)
                    {
                        listBox.Items.Insert(insertIndex + 1, insertItems[insertItems.Count - i - 1]);
                    }
                }

                listBox.SelectedIndex = insertIndex + 1;
            }
        }

        private void HiddenColumnsListBoxKeyUp(object sender, KeyEventArgs e)
        {
            SelectAllListBoxItems(hiddenColumnsListBox, e);
        }

        private void ShowColumnsListBoxKeyUp(object sender, KeyEventArgs e)
        {
            SelectAllListBoxItems(showColumnsListBox, e);
        }

        private static void SelectAllListBoxItems(ListBox list, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.A && e.Control)
            {
                for (int i = 0; i < list.Items.Count; i++)
                {
                    list.SetSelected(i, true);
                }
            }
        }

        private void BtnMoveUpClick(object sender, EventArgs e)
        {
            MoveUp(showColumnsListBox);
        }

        private void BtnMoveDownClick(object sender, EventArgs e)
        {
            MoveDown(showColumnsListBox);
        }

        private void BtnMoveToHideColumnsClick(object sender, EventArgs e)
        {
            MoveSelectedListBoxItems(showColumnsListBox, hiddenColumnsListBox);
        }

        private void BtnMoveToShowColumnsClick(object sender, EventArgs e)
        {
            MoveSelectedListBoxItems(hiddenColumnsListBox, showColumnsListBox);
        }

        private void MoveSelectedListBoxItems(ListBox fromListBox, ListBox toListBox)
        {
            var selectedItems = new List<String>();
            foreach (var item in fromListBox.SelectedItems)
            {
                toListBox.Items.Add(item);
                selectedItems.Add(item.ToString());
            }

            foreach (var item in selectedItems)
            {
                fromListBox.Items.Remove(item);
            }
        }

        private void BtnUpdateResultsClick(object sender, EventArgs e)
        {
            
        }

        private void HiddenColumnsListBoxSelectedIndexChanged(object sender, EventArgs e)
        {
            VerifyAndUpdateHideColumnsSetting();
        }

        private void ShowColumnsListBoxSelectedIndexChanged(object sender, EventArgs e)
        {
            VerifyAndUpdateShowColumnsSetting();
        }
    }
}
