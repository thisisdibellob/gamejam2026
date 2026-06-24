// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameSystemSubsystem.h"
#include "Engine/Engine.h"

void UGameSystemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ResetGameSystem();
}

void UGameSystemSubsystem::WinGame()
{
	FinishGame(EGameSystemResult::Win);
}

void UGameSystemSubsystem::EndGame()
{
	FinishGame(EGameSystemResult::End);
}

void UGameSystemSubsystem::ResetGameSystem()
{
	bIsGameFinished = false;
	GameResult = EGameSystemResult::None;
}

void UGameSystemSubsystem::FindPlayer(AActor* Finder, AActor* FoundPlayer)
{
	UE_LOG(LogTemp, Warning, TEXT("[GameSystem] Player found. Finder: %s, Player: %s"), *GetNameSafe(Finder), *GetNameSafe(FoundPlayer));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Player Found - Game End"));
	}

	OnPlayerFound.Broadcast(Finder, FoundPlayer);
	EndGame();
}

void UGameSystemSubsystem::FinishGame(EGameSystemResult Result)
{
	if (bIsGameFinished)
	{
		return;
	}

	bIsGameFinished = true;
	GameResult = Result;

	if (Result == EGameSystemResult::Win)
	{
		OnGameWon.Broadcast();
	}

	OnGameEnded.Broadcast();
	OnGameFinished.Broadcast(GameResult);
}
