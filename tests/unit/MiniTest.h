// Copyright 2023 Jimmy Eng
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Minimal, dependency-free unit test harness for Comet.
// Covers the subset of gtest used by tests/unit/*.cpp: TEST, TEST_F,
// EXPECT_TRUE/FALSE/EQ/DOUBLE_EQ. No CMake, no vendored framework,
// no NuGet/vcpkg package to resolve -- just this header plus a main()
// compiled directly against libcometsearch.

#pragma once

#include <cmath>
#include <cstdio>

namespace minitest
{
   // Base fixture class: TEST_F's `suite` argument may derive from this
   // (like gtest's ::testing::Test) to get no-op SetUp/TearDown for free.
   struct Test
   {
      void SetUp() {}
      void TearDown() {}
   };

   // Intrusive linked list, not a function-local "magic static" std::vector.
   // Each TestCase global links itself into the list in its own constructor
   // via a plain global head pointer -- the simplest, most primitive
   // registration idiom available.
   struct TestCase
   {
      const char* szSuite;
      const char* szName;
      void (*pFunc)(int&);
      TestCase* pNext;

      TestCase(const char* szSuiteIn, const char* szNameIn, void (*pFuncIn)(int&))
         : szSuite(szSuiteIn), szName(szNameIn), pFunc(pFuncIn), pNext(s_pHead)
      {
         s_pHead = this;
      }

      static TestCase* s_pHead;
   };

   inline TestCase* TestCase::s_pHead = nullptr;

   inline int RunAll()
   {
      int iPassed = 0;
      int iFailed = 0;
      int iTotal = 0;

      for (TestCase* pTest = TestCase::s_pHead; pTest != nullptr; pTest = pTest->pNext)
      {
         int iCaseFailures = 0;
         pTest->pFunc(iCaseFailures);
         ++iTotal;

         if (iCaseFailures == 0)
         {
            ++iPassed;
         }
         else
         {
            ++iFailed;
            std::printf("[FAILED]  %s.%s\n", pTest->szSuite, pTest->szName);
         }
      }

      std::printf("\n%d test(s) passed, %d test(s) failed, %d total.\n",
         iPassed, iFailed, iTotal);

      return iFailed == 0 ? 0 : 1;
   }
}

// Two-level indirection so `suite` and `name` are macro-expanded before
// pasting (a bare `##` only pastes the literal argument tokens, not the
// result of another macro call).
#define MINITEST_CAT_(a, b) a##b
#define MINITEST_CAT(a, b) MINITEST_CAT_(a, b)

#define MINITEST_CLASS(suite, name)     MINITEST_CAT(MINITEST_CAT(suite, _), MINITEST_CAT(name, _Test))
#define MINITEST_REGISTRAR(suite, name) MINITEST_CAT(MINITEST_CAT(suite, _), MINITEST_CAT(name, _Registrar))

// Run() is a static member of the generated fixture subclass (rather than a
// free function) so it can call the base fixture's SetUp()/TearDown() even
// when those are declared protected, matching how gtest's own Test::Run()
// reaches into a protected SetUp()/TearDown() via class membership.
#define TEST_F(suite, name) \
   struct MINITEST_CLASS(suite, name) : public suite \
   { \
      void TestBody(int& iFailures); \
      static void Run(int& iFailures) \
      { \
         MINITEST_CLASS(suite, name) test; \
         test.SetUp(); \
         test.TestBody(iFailures); \
         test.TearDown(); \
      } \
   }; \
   static minitest::TestCase MINITEST_REGISTRAR(suite, name)( \
      #suite, #name, &MINITEST_CLASS(suite, name)::Run); \
   void MINITEST_CLASS(suite, name)::TestBody(int& iFailures)

#define TEST(suite, name) \
   struct MINITEST_CLASS(suite, name) : public minitest::Test \
   { \
      void TestBody(int& iFailures); \
      static void Run(int& iFailures) \
      { \
         MINITEST_CLASS(suite, name) test; \
         test.SetUp(); \
         test.TestBody(iFailures); \
         test.TearDown(); \
      } \
   }; \
   static minitest::TestCase MINITEST_REGISTRAR(suite, name)( \
      #suite, #name, &MINITEST_CLASS(suite, name)::Run); \
   void MINITEST_CLASS(suite, name)::TestBody(int& iFailures)

#define EXPECT_TRUE(cond) \
   do { \
      if (!(cond)) \
      { \
         std::printf("  %s:%d: Expected true: %s\n", __FILE__, __LINE__, #cond); \
         ++iFailures; \
      } \
   } while (0)

#define EXPECT_FALSE(cond) \
   do { \
      if ((cond)) \
      { \
         std::printf("  %s:%d: Expected false: %s\n", __FILE__, __LINE__, #cond); \
         ++iFailures; \
      } \
   } while (0)

// NB: macro-local temporaries use a minitest-prefixed, trailing-underscore
// name (not the more obvious dExpected/dActual) so they can't silently
// shadow a same-named local in the calling TEST_F/TEST body -- a bare
// `double dExpected = (expected);` would self-initialize from its own
// (still-uninitialized) declaration if the caller already had a local
// called dExpected, since the new declaration's scope begins before its
// initializer is evaluated.
#define EXPECT_EQ(expected, actual) \
   do { \
      auto minitestExpectedVal_ = (expected); \
      auto minitestActualVal_ = (actual); \
      if (!(minitestExpectedVal_ == minitestActualVal_)) \
      { \
         std::printf("  %s:%d: Expected %s == %s\n", __FILE__, __LINE__, #expected, #actual); \
         ++iFailures; \
      } \
   } while (0)

#define EXPECT_DOUBLE_EQ(expected, actual) \
   do { \
      double minitestExpectedVal_ = (expected); \
      double minitestActualVal_ = (actual); \
      if (std::fabs(minitestExpectedVal_ - minitestActualVal_) > 1e-9 * (std::fabs(minitestExpectedVal_) + 1.0)) \
      { \
         std::printf("  %s:%d: Expected %s (%.10g) == %s (%.10g)\n", \
            __FILE__, __LINE__, #expected, minitestExpectedVal_, #actual, minitestActualVal_); \
         ++iFailures; \
      } \
   } while (0)

#define MINITEST_MAIN() \
   int main() \
   { \
      return minitest::RunAll(); \
   }
