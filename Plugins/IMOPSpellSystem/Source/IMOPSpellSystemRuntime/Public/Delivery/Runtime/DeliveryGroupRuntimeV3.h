#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "Delivery/DeliveryContextV3.h"
#include "Delivery/Blackboard/DeliveryBlackboardV3.h"

#include "DeliveryGroupRuntimeV3.generated.h"

struct FSpellExecContextV3;

USTRUCT()
struct FDeliveryRigEvalCacheV3
{
	GENERATED_BODY()

	// Minimal cache for Etappe 1: just store root; emitters will be added in Etappe 2 when subsystem evaluates rig.
	UPROPERTY()
	FTransform RootWS = FTransform::Identity;

	UPROPERTY()
	TArray<FTransform> EmittersWS;
};

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryGroupRuntimeV3 : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY()
	FDeliverySpecV3 GroupSpec;

	UPROPERTY()
	FSpellExecContextV3 CtxSnapshot;

	UPROPERTY()
	FDeliveryBlackboardV3 Blackboard;

	UPROPERTY()
	FDeliveryRigEvalCacheV3 RigCache;

	UPROPERTY()
	TMap<FName, TObjectPtr<class UDeliveryDriverBaseV3>> DriversByPrimitiveId;

	UPROPERTY()
	TMap<FName, FDeliveryContextV3> PrimitiveCtxById;

	UPROPERTY()
	int32 GroupSeed = 0;

	UPROPERTY()
	float StartTimeSeconds = 0.f;
};
