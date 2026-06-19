#include "NativeUtils.h"

double NativeSumBuffer(const double* data, size_t n)
{
   double sum = 0.0;
   for (size_t i = 0; i < n; i++)
      sum += data[i];
   return sum;
}
