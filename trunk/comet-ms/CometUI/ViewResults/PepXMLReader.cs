using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml;
using System.Xml.Linq;

namespace CometUI.ViewResults
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
            var reader = CreateXmlReader();
            if (null == reader)
            {
                return null;
            }

            return ReadElements(reader, elementName);
        }

        private XmlTextReader CreateXmlReader()
        {
            if ((String.Empty == FileName) || !File.Exists(FileName))
            {
                return null;
            }

            var reader = new XmlTextReader(FileName);
            reader.MoveToContent();
            return reader;
        }

        public XElement ReadFirstElement(String elementName)
        {
            var reader = CreateXmlReader();
            if (null == reader)
            {
                return null;
            }

            return ReadFirstElement(reader, elementName);
        }

        private XElement ReadFirstElement(XmlTextReader reader, String elementName)
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
                                return el;
                        }
                        break;
                }
            }

            return null;
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

        public XAttribute ReadFirstAttribute(IEnumerable<XElement> elements, String attributeName)
        {
            XAttribute attribute;
            IEnumerable<XAttribute> attributes = ReadAttributes(elements, attributeName);
            try
            {
                attribute = attributes.First();
            }
            catch (Exception)
            {
                attribute = null;
            }

            return attribute;
        }

        public XAttribute ReadFirstAttribute(XElement element, String attributeName)
        {
            XAttribute attribute;
            IEnumerable<XAttribute> attributes = ReadAttributes(element, attributeName);
            try
            {
                attribute = attributes.First();
            }
            catch (Exception)
            {
                attribute = null;
            }

            return attribute;
        }

        public IEnumerable<XAttribute> ReadAttributes(IEnumerable<XElement> elements, String attributeName)
        {
            return elements.Select(element => element.Attribute(attributeName)).Where(attribute => null != attribute);
        }

        public IEnumerable<XAttribute> ReadAttributes(XElement element, String attributeName)
        {
            return element.Attributes().Where(attribute => attribute.Name == attributeName);
        }
    }
}
