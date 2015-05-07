using System;
using System.IO;
using CometWrapper;

namespace CometUI
{
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
                ErrorMessage = "Unable to get the Comet version. Cannot create a params file without a valid Comet version.";
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
