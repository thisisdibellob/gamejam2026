#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "Sound/SoundBase.h"
#include "PatrolNPC2.generated.h"

class UWidgetComponent;

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
	Stunned UMETA(DisplayName = "Stunned"),
	Dead UMETA(DisplayName = "Dead")
};

UCLASS()
class GAMEJAM2026_API APatrolNPC2 : public ACharacter
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "NPC|Events")
	void OnNPCStunStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC|Events")
	void OnNPCDeathStarted();

	UFUNCTION(BlueprintPure, Category = "NPC|Animation")
	bool IsWalkingForAnimation() const;

	UFUNCTION(BlueprintPure, Category = "NPC|Animation")
	bool IsStunnedForAnimation() const;

	UFUNCTION(BlueprintPure, Category = "NPC|Animation")
	bool IsDeadForAnimation() const;

	UFUNCTION(BlueprintCallable, Category = "NPC|Status")
	void SetStunned(bool bNewStunned);

	UFUNCTION(BlueprintCallable, Category = "NPC|Knot")
	void ShowKnotPos();

	UFUNCTION(BlueprintCallable, Category = "NPC|Knot")
	void ShowKnotNeg();

	UFUNCTION(BlueprintCallable, Category = "NPC|Knot")
	void ShowKnotByCrimeState();

	// NPC가 공격당했을 때 호출.
	// 실제 Destroy 대신 숨겼다가 RespawnDelay 후 StartLocation에서 다시 활성화함.
	UFUNCTION(BlueprintCallable, Category = "NPC|Respawn")
	void DisableAndRespawn();

	APatrolNPC2();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Sound")
	USoundBase* HitSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Sound")
	USoundBase* StunSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Sound")
	USoundBase* DeathScreamSound = nullptr;

	// 생성/리스폰될 때 범죄자로 설정될 확률. 0.5이면 50%
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Status", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriminalSpawnChance = 0.5f;
	// 스턴 당했을 때 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Animation")
	UAnimMontage* StunMontage = nullptr;

	// 죽었을 때 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Animation")
	UAnimMontage* DeathMontage = nullptr;
	
	// 죽는 애니메이션이 화면에 보일 시간.
	// 이 시간이 지난 뒤 NPC를 숨김.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Respawn", meta = (ClampMin = "0.0"))
	float DeathHideDelay = 3.0f;

	// 스턴이 유지되는 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Status", meta = (ClampMin = "0.0"))
	float StunDuration = 3.0f;

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
	bool bDrawDebugDetection = false;

	// 정면 기준 감지 부채꼴 전체 각도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Detection", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float DetectionAngle = 60.0f;

	// 부채꼴 안에 몇 개의 레이캐스트를 쏠지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Detection", meta = (ClampMin = "1"))
	int32 DetectionRayCount = 30;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Detection")
	bool bHasDiscoveredPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Detection", meta = (ClampMin = "0.0"))
	float RequiredPlayerDetectionTime = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Detection")
	float PlayerDetectionTimer = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Detection")
	bool bIsDetectingPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Status")
	bool isCrim = false;

	// 공격당해서 사라진 뒤 다시 나타나기까지 걸리는 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 30.0f;

	// 현재 리스폰 대기 중인지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Respawn")
	bool bIsRespawning = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Status")
	FVector FrozenLocation = FVector::ZeroVector;

private:

	FTimerHandle FreezeDeathPoseTimerHandle;

	void FreezeDeathPose();

	FTimerHandle DeathHideTimerHandle;

	// 죽는 애니메이션 출력 후 NPC를 숨기고 리스폰 타이머를 시작함
	void HideAfterDeathAnimation();

	FTimerHandle StunTimerHandle;

	// 스턴 시간이 끝났을 때 다시 순찰/감지를 켬
	void RecoverFromStun();

	FTimerHandle RespawnTimerHandle;

	// RespawnDelay 후 호출되어 NPC를 다시 활성화함
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
	void CheckPlayerDetection(float DeltaTime);

	UWidgetComponent* GetKnotWidgetComponent() const;
	void ShowKnotWithWidgetFunction(FName WidgetFunctionName);
};
