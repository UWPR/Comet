@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cl.exe /EHsc /std:c++17 /MD /IC:\Work\Comet-master\MSToolkit\include /IC:\Work\Comet-master\MSToolkit\include\extern main.cpp /Fe:LegacyRawReaderSpike.exe /link /LIBPATH:C:\Work\Comet-master\x64\Release MSToolkit.lib MSToolkitExtern.lib
