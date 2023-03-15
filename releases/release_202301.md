### Comet releases 2023.01

Documentation for parameters for release 2023.01 [can be found here](/Comet/parameters/parameters_202301/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2023.01 rev. 1 (2023.01.1), release date 2023/03/15
- For pep.xml output when "[num_output_lines = 1](https://uwpr.github.io/Comet/parameters/parameters_202301/num_output_lines.html)", the deltaCn scores will always be reported as "1.0". This occurs when only the top hit is reported and has been this behavior since the original release of Comet. This update will correctly report the deltaCn value for this case. Thanks to J. Scheid for reporting this issue.
- Comet and MSToolkit had input file name/path limits of 512 and 256 characters, respectively. Any input file strings longer than 256 would cause an error in reading the input spectra data. The file name buffer has been expanded to 4096 to mitigate this issue. Thanks to B. Connolly for reporting this issue.
- The Linux/Ubuntu and Mac runners for compiling the release binaries via GitHub Actions have been changed from "ubuntu-latest" and "macos-latest" to "ubuntu-20.04" and "macos-11". This is to give these binaries wider, backwards compatibility with older OS's. Thanks to T. Sachsenberg for reporting this issue.

#### release 2023.01 rev. 0 (2023.01.0), release date 2023/01/31

- Address issue where a peptide with a low xcorr identified in a very poor/sparse spectrum would be assigned a good E-value. This is due to the majority of matched xcorr scores being "0" so any poor scoring match looks like a good outlier. Thanks to D. Shteynberg for reporting the issue.
- Add "[scale_fragmentNL](/Comet/parameters/parameters_202301/scale_fragmentNL.html)" parameter entry which scales (multiplies) the neutral loss mass value by the number of modified residues in the fragment. Feature requested by A. Keller.
- Add contributions of fragment neutral loss peaks in preliminary (Sp) score; previously they only applied to the cross-correlation score.
- Fix bug where the fragment neutral loss peak was not analyzed if the primary fragment peak was not matched.
- Fix bug where Comet files to analyze a variable modification if more than 19 residues are specified for that mod. Thanks to D. Tabb for reporting the issue.
- Fix minor typo in command line help. Thanks to M. Riffle for the pull request.
