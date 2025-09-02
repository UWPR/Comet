#include <sstream>
#include "AScorePeptide.h"

namespace AScoreProCpp
{

   Peptide::Peptide() :
      id_(0),
      scanNumber_(0),
      precursorMz_(0.0),
      score_(0.0),
      leftFlank_('\0'),
      rightFlank_('\0'),
      ionsTotal_(0),
      ionsMatched_(0),
      generatorIndex_(0)
   {
   }

   Peptide Peptide::clone() const
   {
      Peptide copy;
      copy.id_ = id_;
      copy.scanNumber_ = scanNumber_;
      copy.precursorMz_ = precursorMz_;
      copy.score_ = score_;
      copy.leftFlank_ = leftFlank_;
      copy.rightFlank_ = rightFlank_;
      copy.mods_ = mods_.clone();
      copy.ionsTotal_ = ionsTotal_;
      copy.ionsMatched_ = ionsMatched_;
      copy.generatorIndex_ = generatorIndex_;
      copy.matches_ = matches_;
      copy.matchesByDepth_ = matchesByDepth_;
      copy.sequence_ = sequence_;
      copy.annotatedSequence_ = annotatedSequence_;
      return copy;
   }

   std::string Peptide::toString() const
   {
      if (!annotatedSequence_.empty())
      {
         return annotatedSequence_;
      }

      std::string result;

      // Add the left flank and dot if present
      if (leftFlank_ != '\0')
      {
         result += leftFlank_;
         result += '.';
      }

      // Create a working copy of the sequence
      std::string workingSequence = sequence_;

      // Process modifications and insert symbols after their respective amino acids
      // We need to process from right to left to maintain position integrity
      for (int i = static_cast<int>(workingSequence.length()) - 1; i >= 0; --i)
      {
         std::vector<PeptideMod> modsAtPos = mods_.getMods(i);

         // Insert all modification symbols after this position
         for (const auto& mod : modsAtPos)
         {
            workingSequence.insert(i + 1, 1, mod.getSymbol());
         }
      }

      // Add the working sequence to the result
      result += workingSequence;

      // Add the right flank and dot if present
      if (rightFlank_ != '\0')
      {
         result += '.';
         result += rightFlank_;
      }

      // Cache the annotated sequence
      const_cast<Peptide*>(this)->annotatedSequence_ = result;

      return result;
   }

   // Getters and setters implementation
   int Peptide::getId() const
   {
      return id_;
   }

   void Peptide::setId(int id)
   {
      id_ = id;
   }

   int Peptide::getScanNumber() const
   {
      return scanNumber_;
   }

   void Peptide::setScanNumber(int scanNumber)
   {
      scanNumber_ = scanNumber;
   }

   double Peptide::getPrecursorMz() const
   {
      return precursorMz_;
   }

   void Peptide::setPrecursorMz(double precursorMz)
   {
      precursorMz_ = precursorMz;
   }

   double Peptide::getScore() const
   {
      return score_;
   }

   void Peptide::setScore(double score)
   {
      score_ = score;
   }

   char Peptide::getLeftFlank() const
   {
      return leftFlank_;
   }

   void Peptide::setLeftFlank(char leftFlank)
   {
      leftFlank_ = leftFlank;
   }

   char Peptide::getRightFlank() const
   {
      return rightFlank_;
   }

   void Peptide::setRightFlank(char rightFlank)
   {
      rightFlank_ = rightFlank;
   }

   PeptideMods& Peptide::getMods()
   {
      return mods_;
   }

   const PeptideMods& Peptide::getMods() const
   {
      return mods_;
   }

   void Peptide::setMods(const PeptideMods& mods)
   {
      mods_ = mods;
   }

   int Peptide::getIonsTotal() const
   {
      return ionsTotal_;
   }

   void Peptide::setIonsTotal(int ionsTotal)
   {
      ionsTotal_ = ionsTotal;
   }

   int Peptide::getIonsMatched() const
   {
      return ionsMatched_;
   }

   void Peptide::setIonsMatched(int ionsMatched)
   {
      ionsMatched_ = ionsMatched;
   }

   int Peptide::getGeneratorIndex() const
   {
      return generatorIndex_;
   }

   void Peptide::setGeneratorIndex(int generatorIndex)
   {
      generatorIndex_ = generatorIndex;
   }

   std::vector<PeakMatch>& Peptide::getMatches()
   {
      return matches_;
   }

   const std::vector<PeakMatch>& Peptide::getMatches() const
   {
      return matches_;
   }

   void Peptide::setMatches(const std::vector<PeakMatch>& matches)
   {
      matches_ = matches;
   }

   std::vector<int>& Peptide::getMatchesByDepth()
   {
      return matchesByDepth_;
   }

   const std::vector<int>& Peptide::getMatchesByDepth() const
   {
      return matchesByDepth_;
   }

   void Peptide::setMatchesByDepth(const std::vector<int>& matchesByDepth)
   {
      matchesByDepth_ = matchesByDepth;
   }

   const std::string& Peptide::getSequence() const
   {
      return sequence_;
   }

   void Peptide::setSequence(const std::string& sequence)
   {
      sequence_ = sequence;
   }

   const std::string& Peptide::getAnnotatedSequence() const
   {
      if (annotatedSequence_.empty())
      {
         // Using const_cast to allow modification in this specific case
         const_cast<Peptide*>(this)->annotatedSequence_ = toString();
      }
      return annotatedSequence_;
   }

   void Peptide::setAnnotatedSequence(const std::string& annotatedSequence)
   {
      annotatedSequence_ = annotatedSequence;
   }

} // namespace AScoreProCpp
