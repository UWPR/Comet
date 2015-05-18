/*
   Copyright 2015 University of Washington

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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
            if (listBox.SelectedIndex == listBox.Items.Count -1)
            {
                return;
            }

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

        private void HiddenColumnsListBoxSelectedIndexChanged(object sender, EventArgs e)
        {
            VerifyAndUpdateHideColumnsSetting();
            VerifyAndUpdateShowColumnsSetting();
        }

        private void ShowColumnsListBoxSelectedIndexChanged(object sender, EventArgs e)
        {
            VerifyAndUpdateShowColumnsSetting();
            VerifyAndUpdateHideColumnsSetting();
        }

        private void BtnUpdateResultsClick(object sender, EventArgs e)
        {
            ViewSearchResultsControl.UpdateSearchResultsList();
        }
    }
}
