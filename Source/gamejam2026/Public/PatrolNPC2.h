#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PatrolNPC2.generated.h"

// NPC가 어느 방향으로 왕복 이동할지 정하는 enum
UENUM(BlueprintType)
enum class EPatrolNPC2Direction : uint8
{
	Horizontal UMETA(DisplayName = "Horizontal"),
	Vertical UMETA(DisplayName = "Vertical")
};

// NPC의 현재 상태를 나타내는 enum
// 나중에 애니메이션, 대화, 기절, 플레이어 감지 로직에서 사용할 수 있음
UENUM(BlueprintType)
enum class EPatrolNPC2State : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	MovingToEnd UMETA(DisplayName = "Moving To End"),
	WaitingAtEnd UMETA(DisplayName = "Waiting At End"),
	MovingToStart UMETA(DisplayName = "Moving To Start"),
	WaitingAtStart UMETA(DisplayName = "Waiting At Start"),
	Stunned UMETA(DisplayName = "Stunned")
};

UCLASS()
class GAMEJAM2026_API APatrolNPC2 : public ACharacter
{
	GENERATED_BODY()

public:
	APatrolNPC2();

	UFUNCTION(BlueprintCallable, Category = "NPC|Respawn")
	void DisableAndRespawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	// 레벨에 배치한 NPC마다 좌우/상하 이동 방향을 설정할 수 있음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Patrol")
	EPatrolNPC2Direction PatrolDirection = EPatrolNPC2Direction::Horizontal;

	// 시작 위치에서 끝 위치까지의 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Patrol", meta = (ClampMin = "0.0"))
	float PatrolDistance = 200.0f;

	// 초당 이동 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Patrol", meta = (ClampMin = "1.0"))
	float PatrolSpeed = 100.0f;

	// 끝점 또는 시작점에 도착했을 때 멈춰 있는 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Patrol", meta = (ClampMin = "0.0"))
	float WaitTime = 1.0f;

	// false면 순찰하지 않고 현재 상태를 유지함
	// 나중에 대화 중이거나 이벤트 중일 때 끄면 됨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Patrol")
	bool bCanPatrol = true;

	// 현재 NPC 상태
	// Blueprint에서도 읽을 수 있어서 애니메이션 전환 조건으로 사용 가능
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|State")
	EPatrolNPC2State CurrentState = EPatrolNPC2State::Idle;

	// 게임 시작 시 NPC가 레벨에 배치되어 있던 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Patrol")
	FVector StartLocation = FVector::ZeroVector;

	// StartLocation에서 PatrolDirection과 PatrolDistance를 적용한 목표 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Patrol")
	FVector EndLocation = FVector::ZeroVector;

	// 대기 상태에서 남은 시간을 저장하는 타이머
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Patrol")
	float WaitTimer = 0.0f;

	// 플레이어 감지 레이캐스트를 사용할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Detection")
	bool bEnablePlayerDetection = true;

	// NPC가 바라보는 방향으로 얼마나 멀리 검사할지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Detection", meta = (ClampMin = "0.0"))
	float DetectionDistance = 600.0f;

	// 디버그 라인을 화면에 그릴지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Detection")
	bool bDrawDebugDetection = true;

	// 정면 기준 감지 부채꼴 전체 각도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Detection", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float DetectionAngle = 60.0f;

	// 부채꼴 안에 몇 개의 레이캐스트를 쏠지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Detection", meta = (ClampMin = "1"))
	int32 DetectionRayCount = 7;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Detection")
	bool bHasDiscoveredPlayer = false;

	// 공격당해서 사라진 뒤 다시 나타나기까지 걸리는 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 3.0f;

	// 현재 리스폰 대기 중인지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Respawn")
	bool bIsRespawning = false;

private:

	// 대기 시간이 지난 뒤 NPC를 다시 활성화할 때 사용하는 타이머 핸들
	FTimerHandle RespawnTimerHandle;

	void FinishRespawn();
	// 시작 위치와 끝 위치를 계산함
	void SetupPatrolPoints();

	// 현재 위치에서 TargetLocation까지 PatrolSpeed 속도로 이동함
	void MoveToTarget(const FVector& TargetLocation, float DeltaTime);

	// 대기 시간을 줄이고, 시간이 끝나면 반대 방향 이동 상태로 바꿈
	void UpdateWaiting(float DeltaTime);

	// NPC 상태를 변경하는 함수
	void SetNPCState(EPatrolNPC2State NewState);

	// NPC가 바라보는 방향으로 레이캐스트를 쏴서 플레이어를 감지함
	void CheckPlayerDetection();
};