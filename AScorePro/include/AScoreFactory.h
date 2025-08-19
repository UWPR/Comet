#pragma once

#include "API.h"
#include "AScoreDllInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

	// Factory function to create an AScoreDllInterface instance
	ASCORE_API AScoreProCpp::AScoreDllInterface* CreateAScoreDllInterface();

	// Function to delete an AScoreDllInterface instance
	ASCORE_API void DeleteAScoreDllInterface(AScoreProCpp::AScoreDllInterface* instance);

#ifdef __cplusplus
}
#endif