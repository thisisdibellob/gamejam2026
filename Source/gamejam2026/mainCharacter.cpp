// Copyright Epic Games, Inc. All Rights Reserved.

#include "mainCharacter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

AMainCharacter::AMainCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetGuilt(Guilt);
	SetBlood(Blood);
	SetStamina(MaxStamina);

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// 게임 시작 시 첫 변신 스케줄링
	ScheduleNextTransformation();
}

void AMainCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateSprint(DeltaSeconds);

	// 뱀파이어 상태일 때 흡혈 결핍도 지속 상승 (영구 흡혈귀가 아닐 때만)
	if (bIsVampire && PermanentState != EPermanentState::PureVampire)
	{
		float OldBlood = Blood;
		float BloodIncreasePerSecond = 3.0f; // 초당 3씩 결핍도 증가 (기획에 맞게 수정 가능)

		SetBlood(Blood + (BloodIncreasePerSecond * DeltaSeconds));

		// 로그 스팸을 막기 위해 100에 딱 도달한 순간 한 번만 경고 출력
		if (OldBlood < MaxBlood && Blood >= MaxBlood)
		{
			UE_LOG(LogTemp, Warning, TEXT("[System] Blood deficiency reached maximum! Quick hunting is needed."));
		}
	}
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 달리기 (Shift)
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AMainCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMainCharacter::StopSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AMainCharacter::StopSprint);
		}

		// 혈관 검사 (Q)
		if (InspectAction)
		{
			EnhancedInputComponent->BindAction(InspectAction, ETriggerEvent::Started, this, &AMainCharacter::PerformInspect);
		}

		// 기절 (E)
		if (StunAction)
		{
			EnhancedInputComponent->BindAction(StunAction, ETriggerEvent::Started, this, &AMainCharacter::PerformStun);
		}

		// 죽이기/사냥 (R)
		if (KillAction)
		{
			EnhancedInputComponent->BindAction(KillAction, ETriggerEvent::Started, this, &AMainCharacter::PerformKill);
		}
	}
}

void AMainCharacter::SetGuilt(float NewGuilt)
{
	const float OldGuilt = Guilt;
	Guilt = FMath::Clamp(NewGuilt, 0.0f, 100.0f);

	// 영구 상태 체크 및 로그 출력
	if (Guilt >= 100.0f && PermanentState != EPermanentState::PureVampire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Guilt reached 100. Permanently turned into 'Pure Vampire'!"));
		PermanentState = EPermanentState::PureVampire;
		SetIsVampire(true);
	}
	else if (Guilt <= 0.0f && PermanentState != EPermanentState::PureHuman)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Guilt reached 0. Permanently turned into 'Pure Human'!"));
		PermanentState = EPermanentState::PureHuman;
		SetIsVampire(false);
	}

	if (!FMath::IsNearlyEqual(OldGuilt, Guilt))
	{
		OnGuiltChanged(Guilt, OldGuilt);
	}
}

void AMainCharacter::AddGuilt(float Amount)
{
	// 처치한 대상에 따른 로그 출력 (양수: 일반인, 음수: 범죄자)
	if (Amount > 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Killed an innocent. Guilt increased by %f."), Amount);
	}
	else if (Amount < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Killed a criminal. Guilt decreased by %f."), FMath::Abs(Amount));
	}

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
	// 음수값이 들어오면 사냥을 통해 결핍도를 해소한 것으로 판정
	if (Amount < 0.0f && bIsVampire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Bloodsucking successful. Blood deficiency decreased by %f."), FMath::Abs(Amount));
	}

	SetBlood(Blood + Amount);
}

void AMainCharacter::SetIsVampire(bool bNewIsVampire)
{
	if (bIsVampire == bNewIsVampire) return;

	bIsVampire = bNewIsVampire;

	if (bIsVampire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Transformed into a Vampire! Will revert to Human in 30 seconds."));

		if (bIsSprinting) StopSprint();
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

		// 영구 흡혈귀가 아닐 때만 30초 후 돌아가는 타이머 작동
		if (PermanentState != EPermanentState::PureVampire)
		{
			FTimerDelegate TimerDel;
			TimerDel.BindUFunction(this, FName("SetIsVampire"), false);
			GetWorldTimerManager().SetTimer(TransformTimerHandle, TimerDel, 30.0f, false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Reverted to Human. Awaiting next transformation."));

		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		ScheduleNextTransformation(); // 인간이 되었으므로 다음 변신 스케줄링
	}

	OnVampireChanged(bIsVampire);
}

/* --- 달리기 로직 --- */
void AMainCharacter::StartSprint()
{
	if (bIsVampire) return;
	if (Stamina < MinStaminaToSprint) return;

	if (!bIsSprinting)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		OnSprintChanged(true);
	}
}

void AMainCharacter::StopSprint()
{
	if (bIsVampire) return;

	if (bIsSprinting)
	{
		bIsSprinting = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		OnSprintChanged(false);
	}
}

void AMainCharacter::UpdateSprint(float DeltaSeconds)
{
	if (bIsVampire)
	{
		SetStamina(Stamina + StaminaRegenPerSecond * DeltaSeconds);
		return;
	}

	if (bIsSprinting)
	{
		SetStamina(Stamina - SprintStaminaDrainPerSecond * DeltaSeconds);
		if (Stamina <= 0.0f) StopSprint();
	}
	else
	{
		SetStamina(Stamina + StaminaRegenPerSecond * DeltaSeconds);
	}
}

void AMainCharacter::SetStamina(float NewStamina)
{
	Stamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina);
}

/* --- 스킬 로직 --- */
void AMainCharacter::PerformInspect()
{
	if (bIsVampire) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastInspectTime < InspectCooldown) return;

	AActor* TargetNPC = GetClosestNPC();
	if (TargetNPC)
	{
		LastInspectTime = CurrentTime;
		OnInspectNPC(TargetNPC);
	}
}

void AMainCharacter::PerformStun()
{
	AActor* TargetNPC = GetClosestNPC();
	if (TargetNPC)
	{
		OnStunNPC(TargetNPC);
	}
}

void AMainCharacter::PerformKill()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastKillTime < KillCooldown) return;

	AActor* TargetNPC = GetClosestNPC();
	if (TargetNPC)
	{
		LastKillTime = CurrentTime;
		OnKillNPC(TargetNPC);
	}
}

AActor* AMainCharacter::GetClosestNPC()
{
	FVector StartLoc = GetActorLocation();
	FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	bool bHit = GetWorld()->SweepMultiByObjectType(HitResults, StartLoc, StartLoc, FQuat::Identity, FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), Sphere, Params);

	AActor* ClosestNPC = nullptr;
	float MinDistSqr = InteractRadius * InteractRadius;

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor != this)
			{
				float DistSqr = (HitActor->GetActorLocation() - StartLoc).SizeSquared();
				if (DistSqr < MinDistSqr)
				{
					MinDistSqr = DistSqr;
					ClosestNPC = HitActor;
				}
			}
		}
	}
	return ClosestNPC;
}

/* --- 변신 시스템 --- */
void AMainCharacter::ScheduleNextTransformation()
{
	if (PermanentState == EPermanentState::PureHuman || PermanentState == EPermanentState::PureVampire) return;

	GetWorldTimerManager().ClearTimer(TransformTimerHandle);

	float BaseWaitTime = FMath::RandRange(30.0f, 50.0f);
	float GuiltFactor = 1.0f - ((Guilt / 100.0f) * 0.5f);
	float FinalWaitTime = BaseWaitTime * GuiltFactor;

	UE_LOG(LogTemp, Warning, TEXT("[System] Will transform into a Vampire in %f seconds."), FinalWaitTime);
	GetWorldTimerManager().SetTimer(TransformTimerHandle, this, &AMainCharacter::TransformToVampire, FinalWaitTime, false);
}

void AMainCharacter::TransformToVampire()
{
	if (PermanentState == EPermanentState::PureHuman) return;

	SetIsVampire(true);
}

void AMainCharacter::OnDiscoveredByNPC(AActor* NPC)
{
	if (bIsDead) return;

	if (bIsVampire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Discovered by NPC! Game Over."));
		bIsDead = true;

		GetCharacterMovement()->DisableMovement();

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}

		OnDeath();
	}
}

/* --- Percent 반환 헬퍼 --- */
float AMainCharacter::GetGuiltPercent() const { return Guilt / 100.0f; }
float AMainCharacter::GetBloodPercent() const { return MaxBlood > 0.0f ? Blood / MaxBlood : 0.0f; }
float AMainCharacter::GetStaminaPercent() const { return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f; }