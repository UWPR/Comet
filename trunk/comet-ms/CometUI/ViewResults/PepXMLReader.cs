using System;
using System.Xml;
using System.Xml.XPath;

namespace CometUI.ViewResults
{
    public class PepXMLReader
    {
        private const String NamespaceURI = "http://regis-web.systemsbiology.net/pepXML";
        private String FileName { get; set; }
        private XPathDocument PepXmlXPathDoc { get; set; }
        private XPathNavigator PepXmlXPathNav { get; set; }
        private XmlNamespaceManager PepXmlNamespaceMgr { get; set; }

        // This constructor will throw an exception if there is an 
        // error, such as invalid file name. The caller needs to
        // handle the exception.
        public PepXMLReader(String fileName)
        {
            FileName = fileName;
            CreateDocNavigator();
        }

        private void CreateDocNavigator()
        {
            PepXmlXPathDoc = new XPathDocument(FileName);
            PepXmlXPathNav = PepXmlXPathDoc.CreateNavigator();

            // Create prefix<->namespace mappings 
            if (PepXmlXPathNav.NameTable != null)
            {
                PepXmlNamespaceMgr = new XmlNamespaceManager(PepXmlXPathNav.NameTable);
                PepXmlNamespaceMgr.AddNamespace("pepXML", NamespaceURI);
                //int spectraCount = (int)(double)PepXmlXPathNav.Evaluate(("count(/pepXML:msms_pipeline_analysis/pepXML:msms_run_summary/pepXML:spectrum_query)"), nsMgr);
                //MessageBox.Show(spectraCount.ToString());
            }
        }

        private String GetFullNodeName(String name)
        {
            var fullName = String.Empty;
            var nodeNames = name.Split('/');
            foreach (var nodeName in nodeNames)
            {
                if (!nodeName.Equals(String.Empty))
                {
                    fullName += "/" + "pepXML:" + nodeName;
                }
            }

            return fullName;
        }

        public XPathNodeIterator ReadNodes(String name)
        {
            return PepXmlXPathNav.Select(GetFullNodeName(name), PepXmlNamespaceMgr);
        }

        public XPathNodeIterator ReadDescendants(XPathNavigator nodeNav, String name)
        {
            return nodeNav.SelectDescendants(name, NamespaceURI, true);
        }

        public XPathNodeIterator ReadChildren(XPathNavigator nodeNav, String name)
        {
            return nodeNav.SelectChildren(name, NamespaceURI);
        }

        public XPathNavigator ReadFirstMatchingChild(XPathNavigator nodeNav, String name)
        {
            var children = nodeNav.SelectChildren(name, NamespaceURI);
            if (children.Count == 0)
            {
                return null;
            }

            children.MoveNext();
            return children.Current;
        }

        public XPathNavigator ReadFirstMatchingNode(String name)
        {
            var iterator = ReadNodes(name);
            iterator.MoveNext();
            return iterator.Current;
        }

        public String ReadAttribute(XPathNavigator nodeNav, String attributeName)
        {
            return nodeNav.GetAttribute(attributeName, String.Empty);
        }

        public String ReadAttributeFromFirstMatchingNode(String nodeName, String attributeName)
        {
            String attribute = String.Empty;
            var nodeNav = ReadFirstMatchingNode(nodeName);
            attribute += ReadAttribute(nodeNav, attributeName);
            return attribute;
        }
    }
}
