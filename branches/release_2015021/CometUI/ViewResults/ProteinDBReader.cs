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

namespace CometUI.ViewResults
{
    class ProteinDBReader
    {
        private String FileName { get; set; }

        public ProteinDBReader(String fileName)
        {
            FileName = fileName;
        }

        /// <summary>
        /// Reads the protein sequence and its header from the protein database
        /// file specified in FileName.
        /// NOTE: This method throws exceptions the caller must handle.
        /// </summary>
        /// <param name="proteinName"> The name of the protein to look up. </param>
        /// <returns> Sucess: The protein sequence and its header,
        /// Failed: null </returns>
        public String ReadProtein(String proteinName)
        {
            String returnStr = null;
            // Create a StreamReader to read from the protein database file.
            // The using statement also closes the StreamReader.
            using (var sr = new StreamReader(FileName))
            {
                String line;
                // Read and display lines from the file until the end of 
                // the file is reached.
                while ((line = sr.ReadLine()) != null)
                {
                    if (line.Contains(proteinName))
                    {
                        returnStr = line + Environment.NewLine;
                        while (((line = sr.ReadLine()) != null) && (!line.Contains(">")))
                        {
                            returnStr += line;
                        }
                        break;
                    }
                }
            }
           
            return returnStr;
        }
    }
}
