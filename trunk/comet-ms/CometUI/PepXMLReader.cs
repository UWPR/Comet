using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml;
using System.Xml.Linq;

namespace CometUI
{
    public class PepXMLReader
    {
        private String FileName { get; set; }

        public PepXMLReader(String fileName)
        {
            FileName = fileName;
        }

        public IEnumerable<XElement> ReadElements(String elementName)
        {
            if ((String.Empty == FileName) || !File.Exists(FileName))
            {
                return null;
            }

            var reader = new XmlTextReader(FileName);
            reader.MoveToContent();
            return ReadElements(reader, elementName);
        }

        private static IEnumerable<XElement> ReadElements(XmlTextReader reader, String elementName)
        {
            while (reader.Read())
            {
                switch (reader.NodeType)
                {
                    case XmlNodeType.Element:
                        if (reader.Name == elementName)
                        {
                            var el = XElement.ReadFrom(reader) as XElement;
                            if (el != null)
                                yield return el;
                        }
                        break;
                }
            }
        }

        public IEnumerable<XAttribute> ReadAttributes(IEnumerable<XElement> elements, String attributeName)
        {
            return elements.Select(element => element.Attribute(attributeName)).Where(attribute => null != attribute);
        }

        public XAttribute ReadFirstAttribute(IEnumerable<XElement> elements, String attributeName)
        {
            IEnumerable<XAttribute> attributes = ReadAttributes(elements, attributeName);
            return attributes.First();
        }
    }
}
