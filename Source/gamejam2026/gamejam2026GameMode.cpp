// Copyright Epic Games, Inc. All Rights Reserved.

#include "gamejam2026GameMode.h"
#include "gamejam2026Character.h"
#include "UObject/ConstructorHelpers.h"

Agamejam2026GameMode::Agamejam2026GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
