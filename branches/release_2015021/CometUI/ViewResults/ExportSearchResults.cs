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
using System.IO;
using BrightIdeasSoftware;

namespace CometUI.ViewResults
{
    class ExportSearchResults
    {
        public void Export(ObjectListView resultsList, String exportFile)
        {
            using (var file = new StreamWriter(exportFile))
            {
                WriteHeader(file, resultsList);
                WriteResults(file, resultsList);
            }
        }

        private void WriteHeader(StreamWriter file, ObjectListView resultsList)
        {
            var header = String.Empty;
            foreach (OLVColumn column in resultsList.Columns)
            {
                header += column.Text + '\t';
            }

            file.WriteLine(header);
            file.Flush();
        }

        private void WriteResults(StreamWriter file, ObjectListView resultsList)
        {
            foreach (OLVListItem item in resultsList.Items)
            {
                var result = String.Empty;
                foreach (OLVListSubItem subItem in item.SubItems)
                {
                    result += subItem.Text + '\t';
                }

                file.WriteLine(result);
                file.Flush();
            }

        }
    }
}
