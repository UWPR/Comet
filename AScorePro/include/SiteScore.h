#pragma once

#ifndef _SITESCORE_H_
#define _SITESCORE_H_

#include "Peptide.h"
#include "Centroid.h"
#include <vector>

namespace AScoreProCpp
{

   /**
    * Stores score information for a specific modification site
    */
   class SiteScore
   {
public:
      /**
       * Constructor with default values
       */
      SiteScore() : position_(0), ionsMatched_(0), ionsTotal_(0), score_(0.0) {}

      // Getters and setters
      int getPosition() const
      {
         return position_;
      }
      void setPosition(int position)
      {
         position_ = position;
      }

      int getIonsMatched() const
      {
         return ionsMatched_;
      }
      void setIonsMatched(int ionsMatched)
      {
         ionsMatched_ = ionsMatched;
      }

      int getIonsTotal() const
      {
         return ionsTotal_;
      }
      void setIonsTotal(int ionsTotal)
      {
         ionsTotal_ = ionsTotal;
      }

      double getScore() const
      {
         return score_;
      }
      void setScore(double score)
      {
         score_ = score;
      }

      std::vector<Peptide>& getPeptides()
      {
         return peptides_;
      }
      const std::vector<Peptide>& getPeptides() const
      {
         return peptides_;
      }
      void setPeptides(const std::vector<Peptide>& peptides)
      {
         peptides_ = peptides;
      }

      std::vector<std::vector<Centroid>>& getSiteIons()
      {
         return siteIons_;
      }
      const std::vector<std::vector<Centroid>>& getSiteIons() const
      {
         return siteIons_;
      }
      void setSiteIons(const std::vector<std::vector<Centroid>>& siteIons)
      {
         siteIons_ = siteIons;
      }

private:
      // Position of the modification site in the peptide
      int position_;

      // Number of matched site-determining ions
      int ionsMatched_;

      // Total number of possible site-determining ions
      int ionsTotal_;

      // Score value for this site
      double score_;

      // Peptides involved in the site determination
      std::vector<Peptide> peptides_;

      // Site-determining ions for each peptide
      std::vector<std::vector<Centroid>> siteIons_;
   };

} // namespace AScoreProCpp

#endif // _SITESCORE_H_
