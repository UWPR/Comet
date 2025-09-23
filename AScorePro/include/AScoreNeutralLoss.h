#pragma once

#ifndef _ASCORENEUTRALLOSS_H_
#define _ASCORENEUTRALLOSS_H_

#include <string>

namespace AScoreProCpp
{

   /**
    * Stores options for mods that can generate neutral loss fragments
    */
   class NeutralLoss
   {
public:
      NeutralLoss() : mass_(0.0) {}

      // Getters and setters
      double getMass() const
      {
         return mass_;
      }

      void setMass(double mass)
      {
         mass_ = mass;
      }

      const std::string& getResidues() const
      {
         return residues_;
      }

      void setResidues(const std::string& residues)
      {
         residues_ = residues;
      }

private:
      // The neutral loss change in mass for the fragment
      double mass_;

      // A string of modified residues that can generate neutral losses (e.g. "ST")
      std::string residues_;
   };

} // namespace AScoreProCpp

#endif // _ASCORENEUTRALLOSS_H_
