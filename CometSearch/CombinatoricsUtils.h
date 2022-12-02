#ifndef _COMBINATORICSUTILS_H_
#define _COMBINATORICSUTILS_H_

class CombinatoricsUtils
{
public:
   CombinatoricsUtils();
   ~CombinatoricsUtils();

   static int** makeCombinations(int n, int r, int count);
   static int nChooseK(int n, int k);
   static int getCombinationCount(int n, int k);
   static void initBinomialCoefficients(const int n, const int k);
};

#endif // _COMBINATORICSUTILS_H_
