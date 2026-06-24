// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "gamejam2026Character.h"
#include "InputAction.h"
#include "mainCharacter.generated.h"

class UInputAction;
struct FInputActionValue;

// 영구 상태를 정의하기 위한 열거형
UENUM(BlueprintType)
enum class EPermanentState : uint8
{
	None			UMETA(DisplayName = "None"),
	PureHuman		UMETA(DisplayName = "Pure Human (영구 인간)"),
	PureVampire		UMETA(DisplayName = "Pure Vampire (영구 흡혈귀)")
};

UCLASS(Blueprintable)
class AMainCharacter : public Agamejam2026Character
{
	GENERATED_BODY()

public:
	AMainCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 사망 상태 확인용 (중복 사망 방지)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status")
	bool bIsDead = false;

	// NPC가 플레이어를 발견했을 때 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Events")
	void OnDiscoveredByNPC(AActor* NPC);

	// 사망 시 블루프린트로 신호를 보내는 이벤트 (게임 오버 UI, 사망 애니메이션 재생용)
	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnDeath();

	/* --- 상태 변수 (체력 제거됨) --- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Guilt = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Status", meta = (ClampMin = "0.0"))
	float MaxBlood = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status", meta = (ClampMin = "0.0"))
	float Blood = 50.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status")
	bool bIsVampire = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status")
	EPermanentState PermanentState = EPermanentState::None;

	/* --- 이동 및 스태미나 --- */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Sprint", meta = (ClampMin = "0.0"))
	float WalkSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Sprint", meta = (ClampMin = "0.0"))
	float SprintSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Sprint", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Sprint")
	float Stamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Sprint", meta = (ClampMin = "0.0"))
	float SprintStaminaDrainPerSecond = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Sprint", meta = (ClampMin = "0.0"))
	float StaminaRegenPerSecond = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Sprint", meta = (ClampMin = "0.0"))
	float MinStaminaToSprint = 5.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Sprint")
	bool bIsSprinting = false;

	/* --- 스킬 및 쿨타임 --- */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Skills")
	float InspectCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Skills")
	float KillCooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Skills")
	float InteractRadius = 150.0f; // NPC 검사 반경

	/* --- 입력 액션 --- */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction; // Shift

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InspectAction; // Q

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* StunAction; // E

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* KillAction; // R

	/* --- Setter 및 C++ 함수 --- */
	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void SetGuilt(float NewGuilt);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void AddGuilt(float Amount);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void SetBlood(float NewBlood);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void AddBlood(float Amount);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void SetIsVampire(bool bNewIsVampire);

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Status")
	float GetGuiltPercent() const;

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Status")
	float GetBloodPercent() const;

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Sprint")
	float GetStaminaPercent() const;

	/* --- 이벤트 디스패처 (BP 연동용) --- */
	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnGuiltChanged(float NewGuilt, float OldGuilt);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnBloodChanged(float NewBlood, float OldBlood);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnVampireChanged(bool bNewIsVampire);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnSprintChanged(bool bNewIsSprinting);

	// BP에서 시각적 효과나 NPC 함수 호출을 처리하기 위한 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnInspectNPC(AActor* TargetNPC);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnKillNPC(AActor* TargetNPC);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnStunNPC(AActor* TargetNPC);

protected:
	virtual void BeginPlay() override;

	void UpdateSprint(float DeltaSeconds);
	void SetStamina(float NewStamina);

	/* --- 블루프린트에서 호출 가능하도록 UFUNCTION 추가됨 --- */
	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Sprint")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Sprint")
	void StopSprint();

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Skills")
	void PerformInspect(); // Q

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Skills")
	void PerformStun();    // E

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Skills")
	void PerformKill();    // R

	// 시스템 로직
	void ScheduleNextTransformation();
	void TransformToVampire();

	// 가장 가까운 NPC 탐색 헬퍼 함수
	AActor* GetClosestNPC();

private:
	FTimerHandle TransformTimerHandle;
	float LastInspectTime = -999.0f;
	float LastKillTime = -999.0f;
};