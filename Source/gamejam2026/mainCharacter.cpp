// Copyright Epic Games, Inc. All Rights Reserved.

#include "mainCharacter.h"
#include "EnhancedInputComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"

AMainCharacter::AMainCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetHealth(MaxHealth);
	SetGuilt(Guilt);
	SetBlood(Blood);
	SetStamina(MaxStamina);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AMainCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSprint(DeltaSeconds);
}

float AMainCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage <= 0.0f || Health <= 0.0f)
	{
		return 0.0f;
	}

	SetHealth(Health - ActualDamage);

	if (Health <= 0.0f)
	{
		StopSprint();
		OnDeath();
	}

	return ActualDamage;
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AMainCharacter::StartSprint);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AMainCharacter::StopSprint);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AMainCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMainCharacter::StopSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AMainCharacter::StopSprint);
		}
	}
}

void AMainCharacter::SetHealth(float NewHealth)
{
	const float OldHealth = Health;
	Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

	if (!FMath::IsNearlyEqual(OldHealth, Health))
	{
		OnHealthChanged(Health, OldHealth);
	}
}

void AMainCharacter::AddHealth(float Amount)
{
	SetHealth(Health + Amount);
}

void AMainCharacter::SetGuilt(float NewGuilt)
{
	const float OldGuilt = Guilt;
	Guilt = FMath::Clamp(NewGuilt, 0.0f, 100.0f);

	if (!FMath::IsNearlyEqual(OldGuilt, Guilt))
	{
		OnGuiltChanged(Guilt, OldGuilt);
	}
}

void AMainCharacter::AddGuilt(float Amount)
{
	SetGuilt(Guilt + Amount);
}

void AMainCharacter::SetBlood(float NewBlood)
{
	const float OldBlood = Blood;
	Blood = FMath::Clamp(NewBlood, 0.0f, MaxBlood);

	if (!FMath::IsNearlyEqual(OldBlood, Blood))
	{
		OnBloodChanged(Blood, OldBlood);
	}
}

void AMainCharacter::AddBlood(float Amount)
{
	SetBlood(Blood + Amount);
}

void AMainCharacter::SetIsVampire(bool bNewIsVampire)
{
	if (bIsVampire == bNewIsVampire)
	{
		return;
	}

	bIsVampire = bNewIsVampire;
	OnVampireChanged(bIsVampire);
}

void AMainCharacter::StartSprint()
{
	if (Health <= 0.0f || Stamina < MinStaminaToSprint)
	{
		return;
	}

	if (!bIsSprinting)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		OnSprintChanged(true);
	}
}

void AMainCharacter::StopSprint()
{
	if (!bIsSprinting)
	{
		return;
	}

	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	OnSprintChanged(false);
}

float AMainCharacter::GetHealthPercent() const
{
	return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
}

float AMainCharacter::GetGuiltPercent() const
{
	return Guilt / 100.0f;
}

float AMainCharacter::GetBloodPercent() const
{
	return MaxBlood > 0.0f ? Blood / MaxBlood : 0.0f;
}

float AMainCharacter::GetStaminaPercent() const
{
	return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f;
}

void AMainCharacter::UpdateSprint(float DeltaSeconds)
{
	if (Health <= 0.0f)
	{
		return;
	}

	if (bIsSprinting)
	{
		SetStamina(Stamina - SprintStaminaDrainPerSecond * DeltaSeconds);

		if (Stamina <= 0.0f)
		{
			StopSprint();
		}

		return;
	}

	SetStamina(Stamina + StaminaRegenPerSecond * DeltaSeconds);
}

void AMainCharacter::SetStamina(float NewStamina)
{
	Stamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);
}
