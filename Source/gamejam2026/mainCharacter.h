// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "gamejam2026Character.h"
#include "mainCharacter.generated.h"

class UInputAction;
struct FInputActionValue;

UCLASS(Blueprintable)
class AMainCharacter : public Agamejam2026Character
{
	GENERATED_BODY()

public:
	AMainCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Status", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MainCharacter|Status")
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Status", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Guilt = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Status", meta = (ClampMin = "0.0"))
	float MaxBlood = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Status", meta = (ClampMin = "0.0"))
	float Blood = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MainCharacter|Status")
	bool bIsVampire = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction;

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void SetHealth(float NewHealth);

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Status")
	void AddHealth(float Amount);

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

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Sprint")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "MainCharacter|Sprint")
	void StopSprint();

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Status")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Status")
	float GetGuiltPercent() const;

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Status")
	float GetBloodPercent() const;

	UFUNCTION(BlueprintPure, Category = "MainCharacter|Sprint")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnHealthChanged(float NewHealth, float OldHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnGuiltChanged(float NewGuilt, float OldGuilt);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnBloodChanged(float NewBlood, float OldBlood);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnVampireChanged(bool bNewIsVampire);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnSprintChanged(bool bNewIsSprinting);

	UFUNCTION(BlueprintImplementableEvent, Category = "MainCharacter|Events")
	void OnDeath();

protected:
	virtual void BeginPlay() override;

	void UpdateSprint(float DeltaSeconds);
	void SetStamina(float NewStamina);
};
