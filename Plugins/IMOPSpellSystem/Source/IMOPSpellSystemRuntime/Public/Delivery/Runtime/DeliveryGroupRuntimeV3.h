#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "Delivery/DeliveryContextV3.h"

#include "Delivery/Blackboard/DeliveryBlackboardV3.h"

#include "DeliveryGroupRuntimeV3.generated.h"

class UDeliveryDriverBaseV3;

/**
 * Cache of latest rig evaluation (world space).
 */
USTRUCT(BlueprintType)
struct FDeliveryRigCacheV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FTransform RootWS = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TArray<FTransform> EmittersWS;
};

/**
 * Group runtime: owned by DeliverySubsystem.
 * Holds:
 * - Spec + Context snapshot
 * - Blackboard
 * - Rig cache
 * - Per-primitive contexts and drivers
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryGroupRuntimeV3 : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY()
	FDeliverySpecV3 GroupSpec;

	// Exec context snapshot used for ticking/stopping (server authoritative)
	UPROPERTY()
	FSpellExecContextV3 CtxSnapshot;

	UPROPERTY()
	float StartTimeSeconds = 0.f;

	UPROPERTY()
	int32 GroupSeed = 0;

	// Blackboard with phase/owner enforcement
	UPROPERTY()
	FDeliveryBlackboardV3 Blackboard;

	// Latest rig evaluation
	UPROPERTY()
	FDeliveryRigCacheV3 RigCache;

	// Live primitive contexts (updated when rig updates)
	UPROPERTY()
	TMap<FName, FDeliveryContextV3> PrimitiveCtxById;

	// Drivers per primitive
	UPROPERTY()
	TMap<FName, TObjectPtr<UDeliveryDriverBaseV3>> DriversByPrimitiveId;
};
