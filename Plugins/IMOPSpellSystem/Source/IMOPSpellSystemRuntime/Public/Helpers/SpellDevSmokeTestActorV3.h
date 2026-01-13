#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpellDevSmokeTestActorV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API ASpellDevSmokeTestActorV3 : public AActor
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
};
