#include "PatrolNPC2.h"
#include "GameSystemSubsystem.h"
#include "../mainCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"


APatrolNPC2::APatrolNPC2()
{
	// 매 프레임 이동과 대기 상태를 갱신해야 하므로 Tick을 켬
	PrimaryActorTick.bCanEverTick = true;
}

void APatrolNPC2::BeginPlay()
{
	Super::BeginPlay();

	isCrim = FMath::FRand() <= CriminalSpawnChance;

	// NPC가 레벨에 놓인 현재 위치를 기준으로 순찰 시작점과 끝점을 계산
	SetupPatrolPoints();

	// 순찰이 꺼져 있거나 이동 거리가 0이면 가만히 둠
	if (!bCanPatrol || PatrolDistance <= 0.0f)
	{
		SetNPCState(EPatrolNPC2State::Idle);
		return;
	}

	// 게임 시작 후 먼저 끝 위치로 이동
	SetNPCState(EPatrolNPC2State::MovingToEnd);
}

void APatrolNPC2::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState == EPatrolNPC2State::Stunned)
	{
		SetActorLocation(FrozenLocation, false);
		bIsDetectingPlayer = false;
		PlayerDetectionTimer = 0.0f;
		return;
	}

	if (CurrentState == EPatrolNPC2State::Dead)
	{
		SetActorLocation(FrozenLocation, false);
		bIsDetectingPlayer = false;
		PlayerDetectionTimer = 0.0f;
		return;
	}

	if (bIsRespawning)
	{
		return;
	}

	CheckPlayerDetection(DeltaTime);

	if (!bCanPatrol || bIsDetectingPlayer)
	{
		return;
	}

	// 현재 상태에 따라 이동 또는 대기 처리
	switch (CurrentState)
	{
	case EPatrolNPC2State::MovingToEnd:
		MoveToTarget(EndLocation, DeltaTime);
		break;

	case EPatrolNPC2State::MovingToStart:
		MoveToTarget(StartLocation, DeltaTime);
		break;

	case EPatrolNPC2State::WaitingAtEnd:
	case EPatrolNPC2State::WaitingAtStart:
		UpdateWaiting(DeltaTime);
		break;

	default:
		break;
	}
}

void APatrolNPC2::SetupPatrolPoints()
{
	// 시작 위치는 BeginPlay 시점의 액터 위치
	StartLocation = GetActorLocation();

	FVector Offset = FVector::ZeroVector;

	// 언리얼 기본 좌표 기준:
	// X축 이동을 Horizontal, Y축 이동을 Vertical로 사용
	if (PatrolDirection == EPatrolNPC2Direction::Horizontal)
	{
		Offset = FVector(PatrolDistance, 0.0f, 0.0f);
	}
	else
	{
		Offset = FVector(0.0f, PatrolDistance, 0.0f);
	}

	// 순찰 끝 위치 계산
	EndLocation = StartLocation + Offset;
}

void APatrolNPC2::MoveToTarget(const FVector& TargetLocation, float DeltaTime)
{
	if (CurrentState == EPatrolNPC2State::Stunned || CurrentState == EPatrolNPC2State::Dead)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();

	const FVector MoveDirection = TargetLocation - CurrentLocation;
	const FVector FlatMoveDirection = FVector(MoveDirection.X, MoveDirection.Y, 0.0f);

	if (!FlatMoveDirection.IsNearlyZero())
	{
		const FRotator TargetRotation = FlatMoveDirection.Rotation();
		SetActorRotation(TargetRotation);
	}

	// VInterpConstantTo는 PatrolSpeed에 맞춰 일정한 속도로 목표 지점까지 이동시켜 줌
	const FVector NewLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		TargetLocation,
		DeltaTime,
		PatrolSpeed
	);

	// Sweep을 false로 두면 충돌 검사 없이 위치를 이동함
	// 포켓몬식 정해진 경로 이동에는 이쪽이 단순함
	SetActorLocation(NewLocation, false);

	// 목표 지점에 거의 도착했는지 확인
	if (FVector::Dist(NewLocation, TargetLocation) <= 1.0f)
	{
		// 미세한 위치 오차를 없애기 위해 정확히 목표 위치에 맞춤
		SetActorLocation(TargetLocation, false);

		// 끝 위치에 도착했다면 대기 후 시작점으로 돌아감
		if (CurrentState == EPatrolNPC2State::MovingToEnd)
		{
			WaitTimer = WaitTime;
			SetNPCState(EPatrolNPC2State::WaitingAtEnd);
		}
		// 시작 위치에 도착했다면 대기 후 다시 끝 위치로 감
		else if (CurrentState == EPatrolNPC2State::MovingToStart)
		{
			WaitTimer = WaitTime;
			SetNPCState(EPatrolNPC2State::WaitingAtStart);
		}
	}
}

void APatrolNPC2::UpdateWaiting(float DeltaTime)
{
	// 대기 시간을 매 프레임 감소시킴
	WaitTimer -= DeltaTime;

	if (WaitTimer > 0.0f)
	{
		return;
	}

	// 음수로 내려간 타이머를 정리
	WaitTimer = 0.0f;

	// 끝에서 기다린 뒤에는 시작점으로 돌아감
	if (CurrentState == EPatrolNPC2State::WaitingAtEnd)
	{
		SetNPCState(EPatrolNPC2State::MovingToStart);
	}
	// 시작점에서 기다린 뒤에는 끝점으로 이동
	else if (CurrentState == EPatrolNPC2State::WaitingAtStart)
	{
		SetNPCState(EPatrolNPC2State::MovingToEnd);
	}
}

void APatrolNPC2::SetNPCState(EPatrolNPC2State NewState)
{
	// 상태 변경을 한 함수에 모아두면,
	// 나중에 상태 변경 이벤트나 로그를 붙이기 쉬움
	CurrentState = NewState;
}

void APatrolNPC2::CheckPlayerDetection(float DeltaTime)
{

	if (CurrentState == EPatrolNPC2State::Stunned || bIsRespawning)
	{
		bIsDetectingPlayer = false;
		PlayerDetectionTimer = 0.0f;
		return;
	}

	if (!bEnablePlayerDetection || bHasDiscoveredPlayer)
	{
		bIsDetectingPlayer = false;
		PlayerDetectionTimer = 0.0f;
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		bIsDetectingPlayer = false;
		PlayerDetectionTimer = 0.0f;
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		bIsDetectingPlayer = false;
		PlayerDetectionTimer = 0.0f;
		return;
	}

	const FVector TraceStart = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const FVector Forward = GetActorForwardVector();

	const int32 RayCount = FMath::Max(DetectionRayCount, 1);
	const float HalfAngle = DetectionAngle * 0.5f;
	const float AngleStep = RayCount > 1 ? DetectionAngle / static_cast<float>(RayCount - 1) : 0.0f;

	bool bHitPlayer = false;
	AActor* HitActor = nullptr;

	for (int32 RayIndex = 0; RayIndex < RayCount; ++RayIndex)
	{
		const float CurrentAngle = -HalfAngle + (AngleStep * RayIndex);

		const FVector RayDirection = Forward.RotateAngleAxis(CurrentAngle, FVector::UpVector);
		const FVector TraceEnd = TraceStart + (RayDirection * DetectionDistance);

		FHitResult HitResult;

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		const bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			QueryParams
		);

		bool bThisRayHitPlayer = false;

		if (bHit)
		{
			HitActor = HitResult.GetActor();
			bThisRayHitPlayer = (HitActor == PlayerPawn);

			if (bThisRayHitPlayer)
			{
				bHitPlayer = true;
			}
		}

	}

	if (!bHitPlayer)
	{
		bIsDetectingPlayer = false;
		PlayerDetectionTimer = 0.0f;
		return;
	}

	bIsDetectingPlayer = true;
	PlayerDetectionTimer += DeltaTime;
	if (PlayerDetectionTimer < RequiredPlayerDetectionTime)
	{
		return;
	}

	AMainCharacter* MainCharacter = Cast<AMainCharacter>(PlayerPawn);
	if (!MainCharacter)
	{
		return;
	}

	bHasDiscoveredPlayer = true;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameSystemSubsystem* GameSystem = GameInstance->GetSubsystem<UGameSystemSubsystem>())
		{
			GameSystem->FindPlayer(this, PlayerPawn);
		}
	}

	MainCharacter->OnDiscoveredByNPC(this);
}

void APatrolNPC2::DisableAndRespawn()
{
	if (bIsRespawning)
	{
		return;
	}

	bIsRespawning = true;
	FrozenLocation = GetActorLocation();

	SetNPCState(EPatrolNPC2State::Dead);

	OnNPCDeathStarted();

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
	}

	if (DeathScreamSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathScreamSound, GetActorLocation());
	}

	bCanPatrol = false;
	bEnablePlayerDetection = false;
	bIsDetectingPlayer = false;
	PlayerDetectionTimer = 0.0f;

	SetActorLocation(FrozenLocation, false);
	SetActorEnableCollision(false);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	float MontageDuration = DeathHideDelay - 1.0f;

	if (DeathMontage)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			MontageDuration = AnimInstance->Montage_Play(DeathMontage);
		}
	}

	// 몽타주가 끝나서 Idle로 돌아가기 직전에 숨김
	const float HideDelay = FMath::Max(MontageDuration - 0.05f, 0.0f);

	World->GetTimerManager().ClearTimer(FreezeDeathPoseTimerHandle);
	World->GetTimerManager().SetTimer(
		FreezeDeathPoseTimerHandle,
		this,
		&APatrolNPC2::FreezeDeathPose,
		HideDelay,
		false
	);

	World->GetTimerManager().ClearTimer(DeathHideTimerHandle);
	World->GetTimerManager().SetTimer(
		DeathHideTimerHandle,
		this,
		&APatrolNPC2::HideAfterDeathAnimation,
		HideDelay,
		false
	);
}

void APatrolNPC2::FinishRespawn()
{
	// 처음 배치됐던 시작 위치로 되돌림
	SetActorLocation(StartLocation, false);

	// 상태 초기화
	WaitTimer = 0.0f;
	bIsRespawning = false;
	bHasDiscoveredPlayer = false;
	bIsDetectingPlayer = false;
	PlayerDetectionTimer = 0.0f;

	isCrim = FMath::FRand() <= CriminalSpawnChance;

	// 다시 보이게 하고 충돌/순찰/감지를 켬
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	bCanPatrol = true;
	bEnablePlayerDetection = true;
	GetMesh()->bPauseAnims = false;
	// Tick을 다시 켜야 이동과 감지가 재개됨
	SetActorTickEnabled(true);

	// 순찰 포인트 재계산
	SetupPatrolPoints();

	SetNPCState(EPatrolNPC2State::MovingToEnd);

}

bool APatrolNPC2::IsWalkingForAnimation() const
{
	return CurrentState == EPatrolNPC2State::MovingToEnd ||
		CurrentState == EPatrolNPC2State::MovingToStart;
}

void APatrolNPC2::SetStunned(bool bNewStunned)
{
	UWorld* World = GetWorld();

	if (bNewStunned)
	{
		if (StunSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, StunSound, GetActorLocation());
		}

		FrozenLocation = GetActorLocation();

		SetActorLocation(FrozenLocation, false);
		SetNPCState(EPatrolNPC2State::Stunned);

		OnNPCStunStarted();

		bCanPatrol = false;
		bEnablePlayerDetection = false;
		bIsDetectingPlayer = false;
		PlayerDetectionTimer = 0.0f;

		GetCharacterMovement()->StopMovementImmediately(); 
		GetCharacterMovement()->DisableMovement();

		if (StunMontage)
		{
			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Play(StunMontage);
			}
		}

		if (World)
		{
			World->GetTimerManager().ClearTimer(StunTimerHandle);
			World->GetTimerManager().SetTimer( 
				StunTimerHandle,
				this,
				&APatrolNPC2::RecoverFromStun,
				StunDuration,
				false
			);
		}

		return;
	}

	RecoverFromStun();
}

void APatrolNPC2::RecoverFromStun()
{
	if (bIsRespawning || CurrentState == EPatrolNPC2State::Dead)
	{
		return;
	}

	// 스턴 당한 그 자리에서 깨어남
	SetActorLocation(FrozenLocation, false);

	GetMesh()->bPauseAnims = false;

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->StopMovementImmediately();

	bCanPatrol = true;

	bEnablePlayerDetection = true;
	bHasDiscoveredPlayer = false;
	bIsDetectingPlayer = false;
	PlayerDetectionTimer = 0.0f;

	// 중요:
	// 여기서 StartLocation / EndLocation 재계산하지 말 것.
	// 원래 순찰 범위를 유지해야 함.

	// 현재 위치에서 더 가까운 순찰 지점으로 이동 재개
	const float DistanceToStart = FVector::Dist(GetActorLocation(), StartLocation);
	const float DistanceToEnd = FVector::Dist(GetActorLocation(), EndLocation);

	if (DistanceToStart < DistanceToEnd)
	{
		SetNPCState(EPatrolNPC2State::MovingToEnd);
	}
	else
	{
		SetNPCState(EPatrolNPC2State::MovingToStart);
	}
}

bool APatrolNPC2::IsStunnedForAnimation() const
{
	return CurrentState == EPatrolNPC2State::Stunned;
}

bool APatrolNPC2::IsDeadForAnimation() const
{
	const bool bIsDeadState = CurrentState == EPatrolNPC2State::Dead;
	UE_LOG(LogTemp, Warning, TEXT("Anim Check Dead: %d"), bIsDeadState);
	return CurrentState == EPatrolNPC2State::Dead;
}

void APatrolNPC2::HideAfterDeathAnimation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 죽는 애니메이션을 보여준 뒤 화면에서 숨김
	SetActorHiddenInGame(true);

	// 숨겨진 동안 Tick도 끔
	SetActorTickEnabled(false);

	// 숨겨진 뒤 RespawnDelay 후 다시 나타남
	World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	World->GetTimerManager().SetTimer(
		RespawnTimerHandle,
		this,
		&APatrolNPC2::FinishRespawn,
		RespawnDelay,
		false
	);
}

void APatrolNPC2::FreezeDeathPose()
{
	SetActorLocation(FrozenLocation, false);
	GetMesh()->bPauseAnims = true;
}
