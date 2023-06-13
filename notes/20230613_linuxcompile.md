### Notes 2023.06.01

Linux compile issues

- It's assumed that you have gcc/g++ installed.
- Hopefully Comet just compiles on your Linux system when you type "make".  The result is a binary "comet.exe" that is executable.
- If you run into compilation issues, such as seeing "/usr/bin/ld: cannot find -lpthread", that means you are missing components in your build system.
- If that's the case, you'll minimally also want (using "yum" as an example):
  - yum install glibc-devel, glibc-static, libstdc++-devel, libstdc++-static
