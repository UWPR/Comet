#pragma once

#ifndef _BINOMIAL_H_
#define _BINOMIAL_H_

namespace AScoreProCpp {

    /**
     * Class for calculating binomial probabilities
     */
    class Binomial {
    public:
        /**
         * Calculates the binomial probability mass function.
         * P(X = k) = (n choose k) * p^k * (1-p)^(n-k)
         *
         * @param p Probability of success
         * @param n Number of trials
         * @param k Number of successes
         * @return The probability mass P(X = k)
         */
        static double PMF(double p, int n, int k);

        /**
         * Calculates the binomial cumulative distribution function.
         * P(X <= k) = sum(i=0 to k, PMF(p, n, i))
         *
         * @param p Probability of success
         * @param n Number of trials
         * @param k Number of successes
         * @return The cumulative probability P(X <= k)
         */
        static double CDF(double p, int n, int k);

    private:
        /**
         * Computes the logarithm of the gamma function.
         * Used internally for calculating combinations.
         *
         * @param x Input value
         * @return Log of the gamma function
         */
        static double logGamma(double x);

        /**
         * Computes the logarithm of binomial coefficient.
         * log(n choose k) = log(n!) - log(k!) - log((n-k)!)
         *
         * @param n Upper value
         * @param k Lower value
         * @return The log of the binomial coefficient
         */
        static double logBinomialCoeff(int n, int k);
    };

} // namespace AScoreProCpp

#endif // _BINOMIAL_H_
