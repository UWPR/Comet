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

        
        /// <summary>
        /// Constructor for the PepXMLReader that creates the document 
        /// navigator. It throws exceptions that the caller must handle.
        /// such as invalid file name.
        /// </summary>
        /// <param name="fileName"></param>
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

        public XPathNavigator ReadFirstMatchingDescendant(XPathNavigator nodeNav, String name)
        {
            var descendants = nodeNav.SelectDescendants(name, NamespaceURI, true);
            if (descendants.Count == 0)
            {
                return null;
            }

            descendants.MoveNext();
            return descendants.Current;
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

        public bool ReadAttribute<T>(XPathNavigator nodeNavigator, String attributeName, out T attribute)
        {
            attribute = default(T);
            var attributeStrValue = ReadAttribute(nodeNavigator, attributeName);
            if (attributeStrValue.Equals(String.Empty))
            {
                return false;
            }

            attribute = (T)Convert.ChangeType(attributeStrValue, typeof(T));
            return true;
        }

        public String ReadAttributeFromFirstMatchingNode(String nodeName, String attributeName)
        {
            var nodeNav = ReadFirstMatchingNode(nodeName);
            var attribute = ReadAttribute(nodeNav, attributeName);
            return attribute;
        }

        public bool ReadAttributeFromFirstMatchingNode<T>(String nodeName, String attributeName, out T attribute)
        {
            attribute = default(T);
            var attributeStrValue = ReadAttributeFromFirstMatchingNode(nodeName, attributeName);
            if (attributeStrValue.Equals(String.Empty))
            {
                return false;
            }

            attribute = (T) Convert.ChangeType(attributeStrValue, typeof (T));
            return true;
        }
    }
}
