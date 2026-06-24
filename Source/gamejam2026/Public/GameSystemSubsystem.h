// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameSystemSubsystem.generated.h"

class AActor;
class UUserWidget;

UENUM(BlueprintType)
enum class EGameSystemResult : uint8
{
	None UMETA(DisplayName = "None"),
	Win UMETA(DisplayName = "Win"),
	End UMETA(DisplayName = "End")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameSystemSimpleEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameSystemResultEvent, EGameSystemResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGameSystemPlayerFoundEvent, AActor*, Finder, AActor*, FoundPlayer);

UCLASS(BlueprintType)
class GAMEJAM2026_API UGameSystemSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "GameSystem|Events")
	FGameSystemSimpleEvent OnGameWon;

	UPROPERTY(BlueprintAssignable, Category = "GameSystem|Events")
	FGameSystemSimpleEvent OnGameEnded;

	UPROPERTY(BlueprintAssignable, Category = "GameSystem|Events")
	FGameSystemResultEvent OnGameFinished;

	UPROPERTY(BlueprintAssignable, Category = "GameSystem|Events")
	FGameSystemPlayerFoundEvent OnPlayerFound;

	// 게임 승리 조건을 달성했을 때 호출합니다.
	UFUNCTION(BlueprintCallable, Category = "GameSystem")
	void WinGame();

	// 게임 오버, 실패, 강제 종료 등 승리가 아닌 종료 상황에서 호출합니다.
	UFUNCTION(BlueprintCallable, Category = "GameSystem")
	void EndGame();

	// 게임 종료 상태를 초기화해서 다시 승리/종료 처리가 가능하게 만듭니다.
	UFUNCTION(BlueprintCallable, Category = "GameSystem")
	void ResetGameSystem();

	// NPC 등이 플레이어를 발견했을 때 호출합니다.
	UFUNCTION(BlueprintCallable, Category = "GameSystem")
	void FindPlayer(AActor* Finder, AActor* FoundPlayer);

	// 현재 게임이 이미 승리 또는 종료 상태인지 반환합니다.
	UFUNCTION(BlueprintPure, Category = "GameSystem")
	bool IsGameFinished() const { return bIsGameFinished; }

	// 현재 저장된 게임 결과를 반환합니다.
	UFUNCTION(BlueprintPure, Category = "GameSystem")
	EGameSystemResult GetGameResult() const { return GameResult; }

private:
	UPROPERTY()
	bool bIsGameFinished = false;

	UPROPERTY()
	EGameSystemResult GameResult = EGameSystemResult::None;

	UPROPERTY()
	UUserWidget* ActiveGameEndWidget = nullptr;

	// 공통 종료 처리 함수입니다. 중복 실행을 막고 관련 이벤트를 호출합니다.
	void FinishGame(EGameSystemResult Result);

	void ShowGameEndWidget();
	void LockGameInputForUI();
	void RestartCurrentLevel();
};
