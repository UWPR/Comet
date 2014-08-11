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
                if (IsBlankLine(line))
                {
                    continue;
                }

                if (IsCommentLine(line))
                {
                    if (ContainsVersionInfo(line))
                    {
                        if (!IsValidVersion(line))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }

            }

            return true;
        }

        private String ReadLine()
        {
            return _cometParamsReader.ReadLine();
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
            // Todo: Think of a better way to do this, this will break easily if the version line changes in the future
            const int versionIndex = 16;
            String version = line.Substring(versionIndex);

            var searchManager = new CometSearchManagerWrapper();
            String cometVersion = String.Empty;
            if (searchManager.GetParamValue("# comet_version ", ref cometVersion))
            {
                ErrorMessage =
                    "Unable to get the Comet version. Cannot validate the version of the input file.";
                return false;
            }

            return version.Equals(cometVersion);
        }

        private bool IsCommentLine(String line)
        {
            return line.StartsWith("#");
        }

        public void Close()
        {
            _cometParamsReader.Close();
        }
    }

    class CometParamsWriter
    {
        private readonly StreamWriter _cometParamsWriter;

        public String ErrorMessage { get; private set; }

        public CometParamsWriter(String outputFileName)
        {
            _cometParamsWriter = new StreamWriter(outputFileName);
        }

        public bool WriteParamsFile(CometParamsMap paramsMap)
        {
            if (!WriteHeader())
            {
                return false;
            }

            foreach (var pair in paramsMap.CometParams)
            {
                if (pair.Key == "[COMET_ENZYME_INFO]")
                {
                    WriteEnzymeInfoParam(pair.Key, pair.Value.Value);
                }
                else
                {
                    WriteLine(pair.Key + " = " + pair.Value.Value);
                }
            }

            return true;
        }

        public void Close()
        {
            _cometParamsWriter.Close();
        }

        private bool WriteHeader()
        {
            var searchManager = new CometSearchManagerWrapper();
            String cometVersion = String.Empty;
            if (!searchManager.GetParamValue("# comet_version ", ref cometVersion))
            {
                ErrorMessage =
                    "Unable to get the Comet version. Cannot create a params file without a valid Comet version.";
                return false;
            }
            WriteLine("# comet_version " + cometVersion);
            
            return true;
        }

        private void WriteEnzymeInfoParam(String paramName, String paramStrValue)
        {
            WriteLine(paramName);
            String enzymeInfoStr = paramStrValue.Replace(Environment.NewLine, "\n");
            String[] enzymeInfoLines = enzymeInfoStr.Split('\n');
            foreach (var line in enzymeInfoLines)
            {
                if (!String.IsNullOrEmpty(line))
                {
                    String[] enzymeInfoRows = line.Split(',');
                    string enzymeInfoFormattedRow = enzymeInfoRows[0] + ".";
                    for (int i = 1; i < enzymeInfoRows.Length; i++)
                    {
                        enzymeInfoFormattedRow += " " + enzymeInfoRows[i];
                    }

                    WriteLine(enzymeInfoFormattedRow);
                }
            }
        }

        private void WriteLine(String line)
        {
            _cometParamsWriter.WriteLine(line);
            _cometParamsWriter.Flush();
        }
    }
}
