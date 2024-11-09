// Copyright Epic Games, Inc. All Rights Reserved.

#include "CMCTestGameMode.h"
#include "CMCTestCharacter.h"
#include "UObject/ConstructorHelpers.h"

ACMCTestGameMode::ACMCTestGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
