// Copyright Epic Games, Inc. All Rights Reserved.

#include "mainCharacter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Public/PatrolNPC2.h"
#include "Engine/World.h"

AMainCharacter::AMainCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetGuilt(Guilt);
	SetInvest(Invest);
	SetBlood(Blood);
	SetStamina(MaxStamina);

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// ���� ���� �� ù ���� �����ٸ�
	ScheduleNextTransformation();
}

void AMainCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateSprint(DeltaSeconds);

	AActor* NewNearbyNPC = GetClosestNPC();
	if (CurrentNearbyNPC != NewNearbyNPC)
	{
		CurrentNearbyNPC = NewNearbyNPC;
		OnNearbyNPCChangedBroadcast.Broadcast(CurrentNearbyNPC != nullptr, CurrentNearbyNPC);
	}

	// �����̾� ������ �� ���� ���̵� ���� ��� (���� �����Ͱ� �ƴ� ����)
	if (bIsVampire && PermanentState != EPermanentState::PureVampire)
	{
		float OldBlood = Blood;
		float BloodIncreasePerSecond = 3.0f; // �ʴ� 3�� ���̵� ���� (��ȹ�� �°� ���� ����)

		SetBlood(Blood + (BloodIncreasePerSecond * DeltaSeconds));

		// �α� ������ ���� ���� 100�� �� ������ ���� �� ���� ��� ���
		if (OldBlood < MaxBlood && Blood >= MaxBlood)
		{
			UE_LOG(LogTemp, Warning, TEXT("[System] Blood deficiency reached maximum! Quick hunting is needed."));
		}
	}
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &AMainCharacter::TestIncreaseGuilt);
	PlayerInputComponent->BindKey(EKeys::N, IE_Pressed, this, &AMainCharacter::TestIncreaseBlood);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// �޸��� (Shift)
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AMainCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMainCharacter::StopSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AMainCharacter::StopSprint);
		}

		// ���� �˻� (Q)
		if (InspectAction)
		{
			EnhancedInputComponent->BindAction(InspectAction, ETriggerEvent::Started, this, &AMainCharacter::PerformInspect);
		}

		// ���� (E)
		if (StunAction)
		{
			EnhancedInputComponent->BindAction(StunAction, ETriggerEvent::Started, this, &AMainCharacter::PerformStun);
		}

		// ���̱�/��� (R)
		if (KillAction)
		{
			EnhancedInputComponent->BindAction(KillAction, ETriggerEvent::Started, this, &AMainCharacter::PerformKill);
		}
	}
}

void AMainCharacter::TestIncreaseGuilt()
{
	AddGuilt(10.0f);
}

void AMainCharacter::TestIncreaseBlood()
{
	AddBlood(10.0f);
}

void AMainCharacter::SetGuilt(float NewGuilt)
{
	const float OldGuilt = Guilt;
	Guilt = FMath::Clamp(NewGuilt, 0.0f, 100.0f);

	// ���� ���� üũ �� �α� ���
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
		OnGuiltChangedBroadcast.Broadcast(Guilt, OldGuilt);
	}
}

void AMainCharacter::AddGuilt(float Amount)
{
	// óġ�� ��� ���� �α� ��� (���: �Ϲ���, ����: ������)
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

void AMainCharacter::SetInvest(float NewInvest)
{
	const float OldInvest = Invest;
	Invest = FMath::Clamp(NewInvest, 0.0f, 100.0f);

	if (!FMath::IsNearlyEqual(OldInvest, Invest))
	{
		OnInvestChanged(Invest, OldInvest);
		OnInvestChangedBroadcast.Broadcast(Invest, OldInvest);
	}
}

void AMainCharacter::AddInvest(float Amount)
{
	SetInvest(Invest + Amount);
}

void AMainCharacter::SetBlood(float NewBlood)
{
	const float OldBlood = Blood;
	Blood = FMath::Clamp(NewBlood, 0.0f, MaxBlood);

	if (!FMath::IsNearlyEqual(OldBlood, Blood))
	{
		OnBloodChanged(Blood, OldBlood);
		OnBloodChangedBroadcast.Broadcast(Blood, OldBlood);
	}
}

void AMainCharacter::AddBlood(float Amount)
{
	// �������� ������ ����� ���� ���̵��� �ؼ��� ������ ����
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
		if (TransformSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, TransformSound, GetActorLocation());
		}

		UE_LOG(LogTemp, Warning, TEXT("[System] Transformed into a Vampire! Will revert to Human in 30 seconds."));

		if (bIsSprinting) StopSprint();
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

		// ���� �����Ͱ� �ƴ� ���� 30�� �� ���ư��� Ÿ�̸� �۵�
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

		if (RevertTransformSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RevertTransformSound, GetActorLocation());
		}


		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		ScheduleNextTransformation(); // �ΰ��� �Ǿ����Ƿ� ���� ���� �����ٸ�
	}

	OnVampireChanged(bIsVampire);
	OnVampireChangedBroadcast.Broadcast(bIsVampire);
}

/* --- �޸��� ���� --- */
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

/* --- ��ų ���� --- */
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
	if (!bIsVampire) return;

	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
	}

	if (StunMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Playing StunMontage!")); // 이 로그가 찍히는지 확인!
		PlayAnimMontage(StunMontage);
	}
	AActor* TargetNPC = GetClosestNPC();
	if (TargetNPC)
	{
		if (APatrolNPC2* PatrolNPC = Cast<APatrolNPC2>(TargetNPC))
		{
			PatrolNPC->SetStunned(true);
		}

		OnStunNPC(TargetNPC);
	}
}

void AMainCharacter::PerformKill()
{
	if (!bIsVampire) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastKillTime < KillCooldown)
	{
		UE_LOG(LogTemp, Warning, TEXT("Cooling down..."));
		return;
	}

	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, AttackSound, GetActorLocation());
	}

	if (KillMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("Playing KillMontage!")); // 이 로그가 찍히는지 확인!
		PlayAnimMontage(KillMontage);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("KillMontage is NULL!")); // 이게 찍히면 블루프린트 할당이 안 된 것
	}
	AActor* TargetNPC = GetClosestNPC();
	if (TargetNPC)
	{
		LastKillTime = CurrentTime;

		// 몽타주가 에디터에서 제대로 할당되었는지 확인 후 재생합니다.
		// 할당되지 않았는데 재생하려고 하면 게임이 튕길 수 있어서 꼭 검사해야 해요!
		

		UE_LOG(LogTemp, Warning, TEXT("[MainCharacter] NPC killed broadcast fired."));
		OnNPCKilledBroadcast.Broadcast(TargetNPC);
		OnNPCKilledSimpleBroadcast.Broadcast();
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

/* --- ���� �ý��� --- */
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
	if (TransformMontage)
	{
		PlayAnimMontage(TransformMontage);
	}
	SetIsVampire(true);
}

void AMainCharacter::OnDiscoveredByNPC(AActor* NPC)
{

	UE_LOG(LogTemp, Warning, TEXT("[OnDiscoveredByNPC] bIsDead=%d bIsVampire=%d"),
		bIsDead,
		bIsVampire
	);

	if (bIsDead) return;

	if (!bIsVampire)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] Discovered by NPC, but player is human. No game over."));
		return;
	}

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

/* --- Percent ��ȯ ���� --- */
float AMainCharacter::GetGuiltPercent() const { return Guilt / 100.0f; }
float AMainCharacter::GetInvestPercent() const { return Invest / 100.0f; }
float AMainCharacter::GetBloodPercent() const { return MaxBlood > 0.0f ? Blood / MaxBlood : 0.0f; }
float AMainCharacter::GetStaminaPercent() const { return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f; }
