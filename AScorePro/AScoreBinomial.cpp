#include "AScoreBinomial.h"
#include <cmath>
#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AScoreProCpp
{

   double Binomial::PMF(double p, int n, int k)
   {
      if (p < 0.0 || p > 1.0)
      {
         throw std::invalid_argument("Probability p must be between 0 and 1");
      }
      if (n < 0)
      {
         throw std::invalid_argument("Number of trials n must be non-negative");
      }
      if (k < 0 || k > n)
      {
         throw std::invalid_argument("Number of successes k must be between 0 and n");
      }

      // Handle edge cases
      if (k == 0 && p == 0.0) return 1.0;
      if (k == n && p == 1.0) return 1.0;
      if (k > 0 && p == 0.0) return 0.0;
      if (k < n && p == 1.0) return 0.0;

      // Calculate PMF using logarithms to prevent overflow/underflow
      double logPMF = logBinomialCoeff(n, k);
      logPMF += k * std::log(p);
      logPMF += (n - k) * std::log(1.0 - p);

      return std::exp(logPMF);
   }

   double Binomial::CDF(double p, int n, int k)
   {
      if (p < 0.0 || p > 1.0)
      {
         throw std::invalid_argument("Probability p must be between 0 and 1");
      }
      if (n < 0)
      {
         throw std::invalid_argument("Number of trials n must be non-negative");
      }
      if (k < 0)
      {
         throw std::invalid_argument("Number of successes k must be non-negative");
      }

      // Handle edge cases
      if (k >= n) return 1.0;
      if (p == 0.0) return (k >= 0) ? 1.0 : 0.0;
      if (p == 1.0) return (k >= n) ? 1.0 : 0.0;


      // For k = n-1, and large p, use the more efficient complementary calculation
      if (k == n - 1 && p > 0.5)
      {
         // Use complementary calculation which is more numerically stable
         // P(X <= k) = 1 - P(X > k) = 1 - P(X = n)
         // For k = n-1, only need to subtract P(X = n)
         return 1.0 - PMF(p, n, n);
      }


      // Calculate CDF by summing PMF values
      double cdf = 0.0;
      for (int i = 0; i <= k; ++i)
      {
         cdf += PMF(p, n, i);
      }

      // Add numerical stability check - limit to [0,1]
      if (cdf < 0.0) return 0.0;
      if (cdf > 1.0) return 1.0;


      return cdf;
   }

   double Binomial::logGamma(double x)
   {
      // Constants for the Lanczos approximation
      const double c[8] =
      {
         676.5203681218851,
         -1259.1392167224028,
         771.32342877765313,
         -176.61502916214059,
         12.507343278686905,
         -0.13857109526572012,
         9.9843695780195716e-6,
         1.5056327351493116e-7
      };

      if (x < 0.5)
      {
         // Reflection formula
         return std::log(M_PI) - std::log(std::sin(M_PI * x)) - logGamma(1.0 - x);
      }

      double z = x - 1.0;
      double sum = 0.99999999999980993;
      for (int i = 0; i < 8; ++i)
      {
         sum += c[i] / (z + i + 1.0);
      }

      double t = z + 8.0 - 0.5;
      return std::log(std::sqrt(2.0 * M_PI)) + (z + 0.5) * std::log(t) - t + std::log(sum);
   }

   double Binomial::logBinomialCoeff(int n, int k)
   {
      if (k < 0 || k > n)
      {
         return -std::numeric_limits<double>::infinity();
      }

      // Optimize for common cases
      if (k == 0 || k == n)
      {
         return 0.0;
      }

      // Use symmetry to minimize calculations
      if (k > n - k)
      {
         k = n - k;
      }

      // Use log factorial approximation via logGamma
      return logGamma(n + 1.0) - logGamma(k + 1.0) - logGamma(n - k + 1.0);
   }

} // namespace AScoreProCpp
