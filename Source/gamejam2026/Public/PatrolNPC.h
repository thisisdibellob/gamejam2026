#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PatrolNPC.generated.h"

UCLASS()
class GAMEJAM2026_API APatrolNPC : public ACharacter
{
	GENERATED_BODY()

public:
	APatrolNPC();

protected:
	virtual void BeginPlay() override;
};