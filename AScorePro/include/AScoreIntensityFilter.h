#pragma once

#ifndef _ASCOREINTENSITYFILTER_H_
#define _ASCOREINTENSITYFILTER_H_

#include "AScoreScan.h"

namespace AScoreProCpp
{

   /**
    * Removes the lowest intensity peaks from the scan.
    * A fraction of the peaks in the scan will be removed
    * based on the argument passed to the constructor.
    */
   class IntensityFilter
   {
private:
      double fractionToRemove_;

public:
      /**
       * Initializes the filter.
       *
       * @param fractionToRemove The fraction of lowest intensity peaks to remove
       */
      explicit IntensityFilter(double fractionToRemove);

      /**
       * Performs filtering on the scan, replacing the peak list.
       * Peaks are sorted based on Intensity and the
       * lowest intensity peaks removed.
       *
       * @param scan The scan to filter
       */
      void Filter(Scan& scan);
   };

} // namespace AScoreProCpp

#endif // _ASCOREINTENSITYFILTER_H_
