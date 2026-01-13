#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpellHelloSpellTestActorV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API ASpellHelloSpellTestActorV3 : public AActor
{
    GENERATED_BODY()
protected:
    virtual void BeginPlay() override;
};
