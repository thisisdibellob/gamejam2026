// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameSystemSubsystem.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UGameSystemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bIsGameFinished = false;
	GameResult = EGameSystemResult::None;
	ActiveGameEndWidget = nullptr;
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
	ActiveGameEndWidget = nullptr;

	RestartCurrentLevel();
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

	ShowGameEndWidget();
	LockGameInputForUI();
}

void UGameSystemSubsystem::ShowGameEndWidget()
{
	if (ActiveGameEndWidget)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		return;
	}

	TSubclassOf<UUserWidget> GameEndWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WBP/WBP_gameEnd.WBP_gameEnd_C"));
	if (!GameEndWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameSystem] Failed to load WBP_gameEnd."));
		return;
	}

	ActiveGameEndWidget = CreateWidget<UUserWidget>(PlayerController, GameEndWidgetClass);
	if (ActiveGameEndWidget)
	{
		ActiveGameEndWidget->AddToViewport(100);
	}
}

void UGameSystemSubsystem::LockGameInputForUI()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		return;
	}

	PlayerController->SetPause(true);
	PlayerController->bShowMouseCursor = true;
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
}

void UGameSystemSubsystem::RestartCurrentLevel()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (PlayerController)
	{
		PlayerController->SetPause(false);
		PlayerController->SetIgnoreMoveInput(false);
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->bShowMouseCursor = false;
		PlayerController->SetInputMode(FInputModeGameOnly());
	}

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	UGameplayStatics::OpenLevel(World, FName(*CurrentLevelName));
}
