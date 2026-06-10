#include "AScorePeptideMod.h"

namespace AScoreProCpp
{

   PeptideMod::PeptideMod() :
      symbol_('\0'),
      mass_(0.0),
      position_(0),
      isNTerm_(false),
      isCTerm_(false) {}

   char PeptideMod::getSymbol() const
   {
      return symbol_;
   }

   void PeptideMod::setSymbol(char symbol)
   {
      symbol_ = symbol;
   }

   const std::string& PeptideMod::getResidues() const
   {
      return residues_;
   }

   void PeptideMod::setResidues(const std::string& residues)
   {
      residues_ = residues;
   }

   double PeptideMod::getMass() const
   {
      return mass_;
   }

   void PeptideMod::setMass(double mass)
   {
      mass_ = mass;
   }

   int PeptideMod::getPosition() const
   {
      return position_;
   }

   void PeptideMod::setPosition(int position)
   {
      position_ = position;
   }

   bool PeptideMod::getIsNTerm() const
   {
      return isNTerm_;
   }

   void PeptideMod::setIsNTerm(bool isNTerm)
   {
      isNTerm_ = isNTerm;
   }

   bool PeptideMod::getIsCTerm() const
   {
      return isCTerm_;
   }

   void PeptideMod::setIsCTerm(bool isCTerm)
   {
      isCTerm_ = isCTerm;
   }

   bool PeptideMod::applies(char aa) const
   {
      for (char res : residues_)
      {
         if (res == aa)
         {
            return true;
         }
      }
      return false;
   }

   PeptideMod PeptideMod::clone() const
   {
      PeptideMod copy;
      copy.symbol_ = this->symbol_;
      copy.residues_ = this->residues_;
      copy.mass_ = this->mass_;
      copy.position_ = this->position_;
      copy.isNTerm_ = this->isNTerm_;
      copy.isCTerm_ = this->isCTerm_;
      return copy;
   }

} // namespace AScoreProCpp
