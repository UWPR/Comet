using System;
using System.IO;
using CometWrapper;

namespace CometUI
{
    class CometParamsReader
    {
        private readonly StreamReader _cometParamsReader;
        public CometParamsReader(String fileName)
        {
            _cometParamsReader = new StreamReader(fileName);
        }

        public String ReadLine()
        {
            return _cometParamsReader.ReadLine();
        }

        public void Close()
        {
            _cometParamsReader.Close();
        }
    }

    class CometParamsWriter
    {
        private readonly StreamWriter _cometParamsWriter;

        public CometParamsWriter(String outputFileName)
        {
            _cometParamsWriter = new StreamWriter(outputFileName);
        }

        public bool WriteCometParamsFile(CometParamsMap paramsMap)
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
