// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameSystemSubsystem.h"

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
