// Copyright Epic Games, Inc. All Rights Reserved.

#include "MONA_API_Leaderboard.h"

#define LOCTEXT_NAMESPACE "FMONA_API_LeaderboardModule"

void FMONA_API_LeaderboardModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FMONA_API_LeaderboardModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMONA_API_LeaderboardModule, MONA_API_Leaderboard)