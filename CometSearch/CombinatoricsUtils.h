#pragma once
class CombinatoricsUtils
{
public:
   CombinatoricsUtils();
   ~CombinatoricsUtils();

   static int** makeCombinations(int n, int r, unsigned long long count);
   static unsigned long long nChooseK(int n, int k);
   static unsigned long long getCombinationCount(int n, int k);
   static unsigned long long getCombinationCount(int n, int k, int minK);
   static void initBinomialCoefficients(const int n, const int k);
};

