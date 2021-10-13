# Comet MS/MS

Comet is an open source tandem mass spectrometry (MS/MS) sequence database search tool written primarily in C/C++. The original Comet repository lived on [SourceForge](https://sourceforge.net/projects/comet-ms/) since 2012. It was migrated to GitHub on September 2021.

The project website [currently exists on SourceForge](http://comet-ms.sourceforge.net/).

To compile on linux:

- Type 'make'.  This will generate a binary "comet.exe"


To compile with Microsoft Visual Studio:

- use build tools v142 with Microsoft Visual Studio 2019

- First install [MSFileReader from Thermo Fischer Scientific](https://thermo.flexnetoperations.com/control/thmo/login).
  Once registered you will find the software under "Other Software Releases".
  As of 10/19/2020, I use the file "MSFileReader_x64_3.1_SP4.exe".

- Load "Comet.sln" in Visual Studio

- Set the build to "Release" and "x64".

- Right-mouse-click on the "Comet" project and choose "Build".
  This should generate a binary "Comet.exe" in x64/Release.

  OR
  
  If you want to also build CometUI and the RealtmeSearch
  test applications, both C# tools, build the entire solution
  or build each specific application.
