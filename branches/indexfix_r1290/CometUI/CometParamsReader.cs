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
using CometWrapper;

namespace CometUI
{
    class CometParamsReader
    {
        private readonly StreamReader _cometParamsReader;

        public String ErrorMessage { get; private set; }

        public CometParamsReader(String fileName)
        {
            _cometParamsReader = new StreamReader(fileName);
        }

        public bool ReadParamsFile(CometParamsMap paramsMap)
        {
            string line;
            while ((line = ReadLine()) != null)
            {
                // Skip past the blank lines
                if (IsBlankLine(line))
                {
                    continue;
                }

                if (IsCommentLine(line))
                {
                    // We only care about a comment line if it contains version
                    // info, in which case, we make sure the version is valid.
                    if (ContainsVersionInfo(line) && !IsValidVersion(line))
                    {
                        ErrorMessage = "The params file version is not compatible with this version of Comet.";
                        return false;
                    }
                }
                else if (IsEnzymeInfoLine(line))
                {
                    // We should now be in the enzyme info sectio, which
                    // is always at the very end of the file.
                    if (!ReadEnzymeInfo(line, paramsMap))
                    {
                        ErrorMessage = "Failed to read [COMET_ENZYME_INFO] from the params file.";
                        return false;
                    }

                    //// Ezyme info is always the last thing in a Comet params file, 
                    //// so we are now at the end of the file.
                    //break;
                }
                else
                {
                    // If it's not a blank line, or a comment line, or the 
                    // enzyme info line, it must be a regular parameters line
                    if (!ReadParamLine(line, paramsMap))
                    {
                        ErrorMessage = "Failed to read a parameter from the params file.";
                        return false;
                    }
                }
            }

            return true;
        }

        public void Close()
        {
            _cometParamsReader.Close();
        }

        private bool IsBlankLine(String line)
        {
            return String.Empty == line;
        }

        private bool ContainsVersionInfo(String line)
        {
            return line.StartsWith("# comet_version ");
        }

        private bool IsValidVersion(String line)
        {
            const int versionIndex = 16;
            String version = line.Substring(versionIndex);
            
            var searchManager = new CometSearchManagerWrapper();
            bool isValid = false;
            if (!searchManager.ValidateCometVersion(version, ref isValid))
            {
                ErrorMessage =
                    "Error validating the Comet version.";
                return false;
            }

            if (!isValid)
            {
                ErrorMessage =
                    "The version of the input params file is not compatible with this version of Comet.";
                return false;
            }

            return true;
        }

        private bool IsCommentLine(String line)
        {
            String lineWithoutLeadingWhitSpaces = line.TrimStart();
            return lineWithoutLeadingWhitSpaces.StartsWith("#");
        }

        private bool IsEnzymeInfoLine(String line)
        {
            return line.Contains("[COMET_ENZYME_INFO]");
        }

        private bool ReadEnzymeInfo(String line, CometParamsMap paramsMap)
        {
            String enzymeInfoName = line;
            String enzymeInfoValue = String.Empty;
            while ((line = ReadLine()) != null && !IsBlankLine(line))
            {
                enzymeInfoValue += line + Environment.NewLine;
            }

            if (!paramsMap.SetCometParam(enzymeInfoName, enzymeInfoValue))
            {
                return false;
            }

            return true;
        }

        private bool ReadParamLine(String line, CometParamsMap paramsMap)
        {
            // Remove the comment from the end of the line
            int indexOfComment = line.IndexOf('#');
            if (-1 != indexOfComment)
            {
                line = line.Remove(indexOfComment);
            }

            // Trim off the extra white spaces at the end
            line = line.TrimEnd();

            if (String.Empty == line)
            {
                return false;
            }

            // We should now be left with only one equal sign, with the
            // parameter name on the left, and the value on the right.
            string[] paramItems = line.Split('=');
            if (paramItems.Length != 2)
            {
                return false;
            }

            string name = paramItems[0].Trim();
            string value = paramItems[1].Trim();
            if (!paramsMap.SetCometParam(name, value))
            {
                return false;
            }

            return true;
        }

        private String ReadLine()
        {
            return _cometParamsReader.ReadLine();
        }
    }
}
