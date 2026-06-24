#include "PatrolNPC.h"

APatrolNPC::APatrolNPC()
{
	// 이동은 블루프린트 Timeline이 처리하므로 C++ Tick은 필요 없음
	PrimaryActorTick.bCanEverTick = false;
}

void APatrolNPC::BeginPlay()
{
	Super::BeginPlay();

	// 실제 순찰 위치 계산과 이동 시작은 BP_PatrolNPC의 Event Graph에서 처리
}