#include "PatrolNPC2.h"
#include "../mainCharacter.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"


APatrolNPC2::APatrolNPC2()
{
	// 매 프레임 이동과 대기 상태를 갱신해야 하므로 Tick을 켬
	PrimaryActorTick.bCanEverTick = true;
}

void APatrolNPC2::BeginPlay()
{
	Super::BeginPlay();

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

	if (bIsRespawning)
	{
		return;
	}

	// 순찰이 꺼졌거나 기절 상태면 이동 로직을 실행하지 않음
	if (!bCanPatrol || CurrentState == EPatrolNPC2State::Stunned)
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

	CheckPlayerDetection();
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

void APatrolNPC2::CheckPlayerDetection()
{
	if (!bEnablePlayerDetection)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
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

		if (bDrawDebugDetection)
		{
			const FColor LineColor = bThisRayHitPlayer ? FColor::Red : FColor::Green;

			DrawDebugLine(
				World,
				TraceStart,
				TraceEnd,
				LineColor,
				false,
				0.0f,
				0,
				2.0f
			);
		}
	}

	if (!bHitPlayer || bHasDiscoveredPlayer)
	{
		return;
	}

	AMainCharacter* MainCharacter = Cast<AMainCharacter>(PlayerPawn);
	if (!MainCharacter)
	{
		return;
	}

	bHasDiscoveredPlayer = true;

	MainCharacter->OnDiscoveredByNPC(this);
}

void APatrolNPC2::DisableAndRespawn()
{
	if (bIsRespawning)
	{
		return;
	}

	bIsRespawning = true;

	// 순찰과 감지를 멈춤
	bCanPatrol = false;
	bEnablePlayerDetection = false;
	SetNPCState(EPatrolNPC2State::Idle);

	// 화면에서 숨기고 충돌을 꺼서 플레이어와 상호작용하지 않게 함
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// Tick을 끄기 전에 타이머를 먼저 예약
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		RespawnTimerHandle,
		this,
		&APatrolNPC2::FinishRespawn,
		RespawnDelay,
		false
	);

	// Tick을 꺼서 이동/감지 로직을 완전히 멈춤
	SetActorTickEnabled(false);
}

void APatrolNPC2::FinishRespawn()
{
	// 처음 배치됐던 시작 위치로 되돌림
	SetActorLocation(StartLocation, false);

	// 상태 초기화
	WaitTimer = 0.0f;
	bIsRespawning = false;
	bHasDiscoveredPlayer = false;

	// 다시 보이게 하고 충돌/순찰/감지를 켬
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	bCanPatrol = true;
	bEnablePlayerDetection = true;

	// Tick을 다시 켜야 이동과 감지가 재개됨
	SetActorTickEnabled(true);

	// 현재 위치 기준으로 순찰 포인트를 다시 계산
	SetupPatrolPoints();

	SetNPCState(EPatrolNPC2State::MovingToEnd);
}
