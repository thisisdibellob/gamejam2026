// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "gamejam2026Character.h"
#include "InputAction.h"
#include "mainCharacter.generated.h"

class UInputAction;
struct FInputActionValue;

// ���� ���¸� �����ϱ� ���� ������
UENUM(BlueprintType)
enum class EPermanentState : uint8
{
	None			UMETA(DisplayName = "None"),
	PureHuman		UMETA(DisplayName = "Pure Human (���� �ΰ�)"),
	PureVampire		UMETA(DisplayName = "Pure Vampire (���� ������)")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMainCharacterFloatChangedEvent, float, NewValue, float, OldValue);

UCLASS(Blueprintable)
class AMainCharacter : public Agamejam2026Character
{
	GENERATED_BODY()

public:
	AMainCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ��� ���� Ȯ�ο� (�ߺ� ��� ����)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status")
	bool bIsDead = false;

	// NPC�� �÷��̾ �߰����� �� ȣ���� �Լ�
	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Events")
	void OnDiscoveredByNPC(AActor* NPC);

	// ��� �� �������Ʈ�� ��ȣ�� ������ �̺�Ʈ (���� ���� UI, ��� �ִϸ��̼� �����)
	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnDeath();

	/* --- ���� ���� (ü�� ���ŵ�) --- */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Guilt = 50.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Invest = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Status", meta = (ClampMin = "0.0"))
	float MaxBlood = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status", meta = (ClampMin = "0.0"))
	float Blood = 50.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status")
	bool bIsVampire = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status")
	EPermanentState PermanentState = EPermanentState::None;

	/* --- �̵� �� ���¹̳� --- */
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

	/* --- ��ų �� ��Ÿ�� --- */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Skills")
	float InspectCooldown = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Skills")
	float KillCooldown = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Skills")
	float InteractRadius = 150.0f; // NPC �˻� �ݰ�

	/* --- �Է� �׼� --- */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction; // Shift

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InspectAction; // Q

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* StunAction; // E

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* KillAction; // R

	/* --- Setter �� C++ �Լ� --- */
	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void SetGuilt(float NewGuilt);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void AddGuilt(float Amount);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void SetInvest(float NewInvest);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void AddInvest(float Amount);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void SetBlood(float NewBlood);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void AddBlood(float Amount);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void SetIsVampire(bool bNewIsVampire);

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Status")
	float GetGuiltPercent() const;

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Status")
	float GetInvestPercent() const;

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Status")
	float GetBloodPercent() const;

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Sprint")
	float GetStaminaPercent() const;

	/* --- �̺�Ʈ ����ó (BP ������) --- */
	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnGuiltChanged(float NewGuilt, float OldGuilt);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnInvestChanged(float NewInvest, float OldInvest);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnBloodChanged(float NewBlood, float OldBlood);

	UPROPERTY(BlueprintAssignable, Category = "MainCharacter|Events")
	FMainCharacterFloatChangedEvent OnGuiltChangedBroadcast;

	UPROPERTY(BlueprintAssignable, Category = "MainCharacter|Events")
	FMainCharacterFloatChangedEvent OnInvestChangedBroadcast;

	UPROPERTY(BlueprintAssignable, Category = "MainCharacter|Events")
	FMainCharacterFloatChangedEvent OnBloodChangedBroadcast;

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnVampireChanged(bool bNewIsVampire);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnSprintChanged(bool bNewIsSprinting);

	// BP���� �ð��� ȿ���� NPC �Լ� ȣ���� ó���ϱ� ���� �̺�Ʈ
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

	/* --- �������Ʈ���� ȣ�� �����ϵ��� UFUNCTION �߰��� --- */
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

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Debug")
	void TestIncreaseGuilt();

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Debug")
	void TestIncreaseBlood();

	// �ý��� ����
	void ScheduleNextTransformation();
	void TransformToVampire();

	// ���� ����� NPC Ž�� ���� �Լ�
	AActor* GetClosestNPC();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Animations")
	UAnimMontage* KillMontage; // 여기서 몽타주를 저장할 변수를 만듭니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Animations")
	UAnimMontage* TransformMontage;
private:
	FTimerHandle TransformTimerHandle;
	float LastInspectTime = -999.0f;
	float LastKillTime = -999.0f;
};
