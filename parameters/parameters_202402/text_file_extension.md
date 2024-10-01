### Comet parameter: text_file_extension

- This parameter controls whether the output text file, as controlled by the parameter
[output_txtfile](output_txtfile.html),
will have a default ".txt" file extension or a custom extension specified by this parameter.
- If this parameter value is left blank, the output text file will have the default ".txt" file extension.
- To specify a custom extension, set the parameter value to any
string.  For example, a parameter value "csv" will generate text
files of the name "BASENAME.csv".

Example:
```
text_file_extension =
text_file_extension = any_string_you_want_without_spaces
```
