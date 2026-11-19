// Runs several Comet batch searches back to back in ONE process, the way Comet.cpp's main() runs
// one: GetCometSearchManager -> LoadParameters -> DoSearch -> ReleaseCometSearchManager, once per
// parameters file. Built by tests/unit/run_tests.py (T88) from Comet.cpp compiled with main renamed
// to comet_main. Only an embedding host (the RTS wrapper) re-initializes Comet in a process; this
// driver reproduces that for batch searches so the tests can check what a re-init does to the
// process-lifetime tables (the E-value decoy tables in CometPostAnalysis.cpp).
//
//   reinit_driver <output_prefix> <input_file> <params1> [<params2> ...]
//
// Session N writes <output_prefix>_sN.<ext>. A session that fails makes comet_main call exit(1),
// which ends the driver (rc 1) after the earlier sessions' output has been written.
#include <cstdio>
#include <string>

extern int comet_main(int argc, char* argv[]);

int main(int argc, char** argv)
{
   if (argc < 4)
   {
      fprintf(stderr, "usage: %s <output_prefix> <input_file> <params1> [<params2> ...]\n", argv[0]);
      return 2;
   }

   const std::string strPrefix = argv[1];
   for (int iSession = 1; iSession <= argc - 3; ++iSession)
   {
      const std::string strParams = std::string("-P") + argv[2 + iSession];
      const std::string strName = std::string("-N") + strPrefix + "_s" + std::to_string(iSession);
      char* argvSession[] = { (char*)"comet", (char*)strParams.c_str(), (char*)strName.c_str(), argv[2], nullptr };

      fprintf(stderr, "=== session %d: %s %s\n", iSession, strParams.c_str(), strName.c_str());
      const int iRC = comet_main(4, argvSession);
      fprintf(stderr, "=== session %d rc %d\n", iSession, iRC);
   }

   return 0;
}
