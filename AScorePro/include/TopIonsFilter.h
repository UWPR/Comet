#pragma once

#ifndef _TOPIONSFILTER_H_
#define _TOPIONSFILTER_H_

#include "Scan.h"

namespace AScoreProCpp
{

   /**
    * Filters for top ions per m/z window.
    */
   class TopIonsFilter
   {
private:
      int depth_;
      double window_;

public:
      /**
       * Constructor
       *
       * @param depth Number of peaks to keep per window, based on intensity ranking
       * @param window Window size in m/z units
       */
      TopIonsFilter(int depth, double window);

      /**
       * This filter separates the scan into several m/z windows and
       * within each window ranks the peaks by intensity. Peaks ranking below
       * the input peak depth are then removed.
       *
       * @param scan The scan to filter
       */
      void Filter(Scan& scan);
   };

} // namespace AScoreProCpp

#endif // _TOPIONSFILTER_H_
