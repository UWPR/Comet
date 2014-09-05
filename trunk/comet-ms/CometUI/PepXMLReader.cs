using System;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;
using System.Xml;
using System.Xml.Linq;

namespace CometUI
{
    public class PepXMLReader
    {
        public bool Read(String file)
        {
            if (!File.Exists(file))
            {
                return false;
            }

            var reader = new XmlTextReader(file);
            reader.MoveToContent();

            //IEnumerable<XElement> elements = ReadElement(reader, "sample_enzyme");
            //foreach (var xElement in elements)
            //{
            //    IEnumerable<XAttribute> attributes = xElement.Attributes();
            //    foreach (var attribute in attributes)
            //    {
            //        MessageBox.Show(attribute.Name + " = " + attribute.Value);
            //    }

            //    var children = xElement.Elements();
            //    foreach (var element in children)
            //    {
            //        IEnumerable<XAttribute> subElementAttributes = element.Attributes();
            //        foreach (var subElementAttribute in subElementAttributes)
            //        {
            //            MessageBox.Show(subElementAttribute.Name + " = " + subElementAttribute.Value);
            //        }
            //    }
            //}


            IEnumerable<XElement> elements = ReadElement(reader, "search_hit");
            foreach (var element in elements)
            {
                if ((int)element.Attribute("hit_rank") == 1)
                {
                    IEnumerable<XElement> firstHitElements = element.Elements();
                    foreach (var firstHitElement in firstHitElements)
                    {
                        IEnumerable<XAttribute> attributes = firstHitElement.Attributes();
                        //foreach (var attribute in attributes)
                        //{
                        //    MessageBox.Show(attribute.Name + " = " + attribute.Value);
                        //}
                    }
                }
            }
            
            return true;
        }

        static IEnumerable<XElement> ReadElement(XmlTextReader reader, String elementName)
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
    }
}
