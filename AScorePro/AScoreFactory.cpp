#include "AScoreFactory.h"

// Factory function to create an AScoreDllInterface instance
extern "C" ASCORE_API AScoreProCpp::AScoreDllInterface* CreateAScoreDllInterface()
{
   return new AScoreProCpp::AScoreDllInterface();
}

// Function to delete an AScoreDllInterface instance
extern "C" ASCORE_API void DeleteAScoreDllInterface(AScoreProCpp::AScoreDllInterface* instance)
{
   delete instance;
}