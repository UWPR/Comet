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
using System.Globalization;
using System.Windows.Forms;

namespace CometUI.SharedUI
{
    public class NumericTextBox : TextBox
    {
        // Restricts the entry of characters to digits (including hex), the negative sign, 
        // the decimal point, and editing keystrokes (backspace). 
        protected override void OnKeyPress(KeyPressEventArgs e)
        {
            base.OnKeyPress(e);

            NumberFormatInfo numberFormatInfo = CultureInfo.CurrentCulture.NumberFormat;
            string decimalSeparator = numberFormatInfo.NumberDecimalSeparator;
            string groupSeparator = numberFormatInfo.NumberGroupSeparator;
            string negativeSign = numberFormatInfo.NegativeSign;

            // Workaround for groupSeparator equal to non-breaking space 
            if (groupSeparator == ((char)160).ToString(CultureInfo.InvariantCulture))
            {
                groupSeparator = " ";
            }

            string keyInput = e.KeyChar.ToString(CultureInfo.InvariantCulture);

            if (Char.IsDigit(e.KeyChar))
            {
                // Digits are OK
            }
            else if ((keyInput.Equals(decimalSeparator) && AllowDecimal) || 
                     (keyInput.Equals(groupSeparator) && AllowGroupSeparator) ||
                     (keyInput.Equals(negativeSign) && AllowNegative))
            {
                // Decimal separator is OK
            }
            else if (e.KeyChar == '\b')
            {
                // Backspace key is OK
            }
                //    else if ((ModifierKeys & (Keys.Control | Keys.Alt)) != 0) 
                //    { 
                //     // Let the edit control handle control and alt key combinations 
                //    } 
            else if (AllowSpace && e.KeyChar == ' ')
            {

            }
            else
            {
                // Consume this invalid key and beep
                e.Handled = true;
                //    MessageBeep();
            }
        }

        public int IntValue
        {
            get
            {
                return Int32.Parse(Text);
            }
        }

        public decimal DecimalValue
        {
            get
            {
                return Decimal.Parse(Text);
            }
        }

        public bool AllowSpace { get; set; }
        public bool AllowDecimal { get; set; }
        public bool AllowNegative { get; set; }
        public bool AllowGroupSeparator { get; set; }
    }
}
