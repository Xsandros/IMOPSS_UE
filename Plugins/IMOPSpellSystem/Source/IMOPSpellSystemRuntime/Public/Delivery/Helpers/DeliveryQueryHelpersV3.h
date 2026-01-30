#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h" // FHitResult, FOverlapResult, FCollisionQueryParams, FCollisionObjectQueryParams, ECollisionChannel
#include "Delivery/DeliveryTypesV3.h" // FDeliveryQuerySpecV3, enums, etc.
//#include "DeliverySpecV3.generated.h"

// Forward declare to avoid heavy includes / circular deps.
struct FSpellExecContextV3;
struct FDeliveryQuerySpecV3;

struct FDeliveryQueryHelpersV3
{
	static void BuildQueryParams(
		const FSpellExecContextV3& Ctx,
		const FDeliveryQueryPolicyV3& Query,
		FCollisionQueryParams& OutParams);

	static FCollisionObjectQueryParams BuildObjectQueryParams(const FDeliveryQueryPolicyV3& Query);

	static ECollisionChannel GetTraceChannel(const FDeliveryQueryPolicyV3& Query);

	static FName GetProfileNameOrDefault(const FDeliveryQueryPolicyV3& Query, const TCHAR* DefaultProfile);

	static bool OverlapMulti(
		UWorld* World,
		const FDeliveryQueryPolicyV3& Query,
		const FVector& Center,
		const FQuat& Rot,
		const FCollisionShape& Shape,
		const FCollisionQueryParams& Params,
		TArray<FOverlapResult>& OutOverlaps,
		const TCHAR* DefaultProfileForByProfile);

	static bool SweepMulti(
		UWorld* World,
		const FDeliveryQueryPolicyV3& Query,
		const FVector& Start,
		const FVector& End,
		const FQuat& Rot,
		const FCollisionShape& Shape,
		const FCollisionQueryParams& Params,
		TArray<FHitResult>& OutHits,
		const TCHAR* DefaultProfileForByProfile);

	static bool LineTraceMulti(
		UWorld* World,
		const FDeliveryQueryPolicyV3& Query,
		const FVector& Start,
		const FVector& End,
		const FCollisionQueryParams& Params,
		TArray<FHitResult>& OutHits,
		const TCHAR* DefaultProfileForByProfile);

	static void PostFilterByObjectTypes(
		const FDeliveryQueryPolicyV3& Query,
		TArray<FOverlapResult>& InOutOverlaps);

	static void PostFilterByObjectTypes(
		const FDeliveryQueryPolicyV3& Query,
		TArray<FHitResult>& InOutHits);
};
