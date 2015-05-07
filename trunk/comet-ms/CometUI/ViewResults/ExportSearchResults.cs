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
