# CSV2XML Converter

Small CLI tool to convert csv files into xml files.

This project was created as educational, but looks fine enough to upload it.
Compiled only for Windows, but probably will work fine for any other os.

# How To Use
1. Download executable file from releases
   OPTIONAL: Add file to PATH to skip path to executable when using
2. Basic use: `csv2xml input.csv output.xml` (assuming you added file to PATH)

Advanced usage :

Usage example: `csv2xml.exe [-tichf] [-s ';'] input.csv [output.xml]`

* -t - to generate file in table form
* -i - to generate file in inline form
* -c - to combine data from multiple files into one output
* -s '<character>' - to specify separator for csv file (by default ';')
* -h - to skip parsing headers from file, columns will have names like: c1, c2...
* -f - to only parse headers from first file (use only when -c is specified), other files will be treated as continuation of first file (if -h specified this option disables)
* input.csv - one or more csv file(s) to convert
* output.xml - optional output filename. If not specified will be generated from input filename.
* If two or more input files are specified:
  * If equal number of output files are specified or no output file is specified: each file will be converted to each corresponding output file
  * If number of output files less then numbers of input files: Input files that have corresponding output file will be converted into them, other output filenames will be generated automatically
  * If only one output file is specified and -c argument is passed: All content (even if headers are different) will be combined into single specified output file
    * If -c argument is passed and multiple output files: Program will ignore all output files except first one and work as if only one output file is passed
    * If -c argument is passed and no output file is specified: Program will generate new .xml output file based on filename of first input file

## Preview

<img width="686" height="270" alt="program preview" src="https://github.com/user-attachments/assets/d0db5c55-563f-4466-b2e5-9bd471644724" />


