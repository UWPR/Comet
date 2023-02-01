### Comet releases 2023.01

Documentation for parameters for release 2023.01 [can be found here](/Comet/parameters/parameters_202301/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2023.01 rev. 0 (2023.01.0), release date 2023/01/31

- Address issue where a peptide with a low xcorr identified in a very poor/sparse spectrum would be assigned a good E-value. This is due to the majority of matched xcorr scores being "0" so any poor scoring match looks like a good outlier. Thanks to D. Shteynberg for reporting the issue.
- Add "[scale_fragmentNL](/Comet/parameters/parameters_202301/scale_fragmentNL.html)" parameter entry which scales (multiplies) the neutral loss mass value by the number of modified residues in the fragment. Feature requested by A. Keller.
- Add contributions of fragment neutral loss peaks in preliminary (Sp) score; previously they only applied to the cross-correlation score.
- Correct bug where the fragment neutral loss peak was not analyzed if the primary fragment peak was not matched.
- Fix minor typo in command line help. Thanks to M. Riffle for the pull request.
