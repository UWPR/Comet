#pragma once

#ifndef _PEAKMATCH_H_
#define _PEAKMATCH_H_

namespace AScoreProCpp
{

/**
 * Stores information about a matched peak
 */
struct PeakMatch
{
   double theoMz;      // Theoretical m/z
   double obsMz;       // Observed m/z
   double intensity;   // Peak intensity
   int rank;           // Peak rank
};

} // namespace AScoreProCpp

#endif // _PEAKMATCH_H_
