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
using System.Diagnostics;
using System.Windows.Forms;
using CometUI.Properties;
using CometWrapper;
using System.Deployment.Application;

namespace CometUI
{
    public partial class AboutDlg : Form
    {
        public AboutDlg()
        {
            InitializeComponent();

            SetAboutText();

            SetCometVersion();

            SetLinks();
        }

        public Version AssemblyVersion
        {
           get
           {
               Version version;
               try
               {
                   version = ApplicationDeployment.CurrentDeployment.CurrentVersion;
               }
               catch (Exception)
               {
                   // If this application hasn't been one click deployed yet,
                   // "ApplicationDeployment" will thrown an exception, so 
                   // default to the current Assembly version.
                   version = typeof(CometUIMainForm).Assembly.GetName().Version;
               }

               return version;
           }
        }

        private void SetAboutText()
        {
            textBoxAboutComet.Text =
                Resources.AboutDlg_SetAboutText_Comet_UI_provides_a_graphical_user_interface_to_run_Comet_searches__set_and_save_search_parameters__and_import_and_export_the_parameters_;

        }

        private void SetCometVersion()
        {
            var searchManager = new CometSearchManagerWrapper();
            String cometVersion = String.Empty;
            if (searchManager.GetParamValue("# comet_version ", ref cometVersion))
            {
                labelCometEngineVersion.Text = cometVersion;
            }

            labelCometUIVersion.Text = AssemblyVersion.ToString(4);
        }

        private void SetLinks()
        {
            const string uwprLink = "http://proteomicsresource.washington.edu/";
            linkLabelUWPR.Links.Add(0, uwprLink.Length, uwprLink);
        }

        private void BtnOkClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
        }

        private void LinkLabelUWPRLinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start(e.Link.LinkData.ToString());
        }
    }
}
