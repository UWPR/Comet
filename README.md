<img src="https://uwpr.github.io/Comet/images/cometlogo_1_small.png" align="right">

# Comet MS/MS

Comet is an open source tandem mass spectrometry (MS/MS) sequence database search tool written primarily in C/C++. The original Comet repository lived on [SourceForge](https://sourceforge.net/projects/comet-ms/) since 2012. It was migrated to GitHub on September 2021.

The project website [can be found here](https://uwpr.github.io/Comet/). This includes release notes and search parameters documentation.

To compile on linux and macOS:

- Type 'make'.  This will generate a binary "comet.exe".

To compile with Microsoft Visual Studio:

- We current use build tools v143 with Microsoft Visual Studio 2022.

- First install [MSFileReader from Thermo Fischer Scientific](https://uwpr.github.io/Comet/notes/20220228_rawfile.html).

- Load "Comet.sln" in Visual Studio.

- Set the build to "Release" and "x64".

- Right-mouse-click on the "Comet" project and choose "Build". This should generate a binary "Comet.exe" in x64/Release.

Comet integrates:
- Mike Hoopmann's [MSToolkit library](https://github.com/mhoopmann/mstoolkit) for reading various file formats.
- Matthew Belmonte's C implementation of the Phillip J Chase's [Twiddle algorithm](https://www.netlib.org/toms-2014-06-10/382) used in generating modification permutations.
- C++ port of Gygi Lab's [AScorePro](https://github.com/gygilab/MPToolkit/) for modification localization. 
