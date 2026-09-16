<img src="https://uwpr.github.io/Comet/images/cometlogo_1_small.png" align="right">

# Comet MS/MS

Comet is an open source tandem mass spectrometry (MS/MS) sequence database search tool written primarily in C/C++. The original Comet repository lived on [SourceForge](https://sourceforge.net/projects/comet-ms/) since 2012. It was migrated to GitHub on September 2021.

The project website [can be found here](https://uwpr.github.io/Comet/). This includes release notes and search parameters documentation.

To compile on linux and macOS:

- Type 'make'.  This will generate a binary "comet.exe".

To compile with Microsoft Visual Studio:

- We current use build tools v143 with Microsoft Visual Studio 2022.

- Load "Comet.sln" in Visual Studio.

- Set the build to "Release" and "x64".

- Right-mouse-click on the "Comet" project and choose "Build". This should generate a binary "Comet.exe" in x64/Release.

Comet integrates:
- Mike Hoopmann's [MSToolkit library](https://github.com/mhoopmann/mstoolkit) for reading various file formats.
- Matthew Belmonte's C implementation of the [Twiddle algorithm](https://www.netlib.org/toms-2014-06-10/382) used in generating modification permutations.
- C++ port of Gygi Lab's [AScorePro](https://github.com/gygilab/MPToolkit/) for modification localization.
- Barak Shoshany's [BS::thread_pool C++ thread pool library](https://github.com/bshoshany/thread-pool).
- Thermo Fischer's [RawFileReader .Net assembly](https://github.com/thermofisherlsms/RawFileReader).

### Reading Thermo .raw files on Windows

As of Comet release v2026.02 rev. 1, `comet.win64.exe` reads Thermo `.raw` files
directly using Thermo's RawFileReader .NET library. This requires two DLLs to
be present in the **same directory as `Comet.exe`**:

- `ThermoFisher.CommonCore.Data.dll`
- `ThermoFisher.CommonCore.RawFileReader.dll`

No installation or registration step is needed -- just place the two files next to `Comet.exe`
(or in the same folder you run Comet from). This is a deliberate improvement over the older
MSFileReader COM library Comet used previously, which required a separate installer that
registered COM components in the Windows registry.

These DLLs can be obtained from Thermo's official
[thermofisherlsms/RawFileReader](https://github.com/thermofisherlsms/RawFileReader) GitHub repo
(the `Libs/Net471` folder matches the .NET Framework version Comet is built against). Comet has
been built and tested against version `5.0.0.93`. These two DLLs are also
available as release assets in this repository.

`.raw` support is Windows-only. mzXML, mzML, and mgf/ms1/ms2-family inputs work on every platform
without any of the above and require no additional files.

RawFileReader reading tool. Copyright © 2016 by Thermo Fisher Scientific, Inc. All rights reserved.
