#include "PeptideBuilder.h"
#include <sstream>
#include <cctype>

namespace AScoreProCpp
{

PeptideBuilder::PeptideBuilder(const std::vector<PeptideMod>& mods)
{
   for (const auto& mod : mods)
   {
      mods_[mod.getSymbol()] = mod;
   }
}

Peptide PeptideBuilder::build(const std::string& peptideStr) const
{
   Peptide output;
   if (peptideStr.empty())
   {
      return output;
   }

   size_t start = 0;
   if (peptideStr.length() > 1 && peptideStr[1] == '.')
   {
      output.setLeftFlank(peptideStr[0]);
      start = 2;
   }

   size_t end = peptideStr.length();
   if (peptideStr.length() > 1 && peptideStr[peptideStr.length() - 2] == '.')
   {
      output.setRightFlank(peptideStr[peptideStr.length() - 1]);
      end = peptideStr.length() - 2;
   }

   // Extract the sequence and track modification positions
   std::string sequence;
   std::vector<std::pair<int, char>> modPositions;

   for (size_t i = start; i < end; ++i)
   {
      char c = peptideStr[i];
      if (std::isalpha(c))
      {
         sequence += c;
      }
      else
      {
         // Store modification symbol and its position in the sequence
         // The position is the index of the last amino acid added
         if (sequence.length() > 0)
         {
            modPositions.push_back({ static_cast<int>(sequence.length() - 1), c });
         }
      }
   }

   // Set the processed sequence
   output.setSequence(sequence);

   // Add modifications with correct positions
   for (const auto& modPair : modPositions)
   {
      int pos = modPair.first;
      char symbol = modPair.second;

      auto modIt = mods_.find(symbol);
      if (modIt != mods_.end())
      {
         PeptideMod mod = modIt->second;
         mod.setPosition(pos);
         output.getMods().add(pos, mod);
      }
   }

   return output;
}

} // namespace AScoreProCpp