#include "Delivery/Helpers/DeliveryQueryHelpersV3.h"
#include "GameFramework/Actor.h"
#include "Core/SpellExecContextHelpersV3.h" // falls dort FSpellExecContextV3 deklariert ist
#include "Engine/World.h"
#include "Engine/EngineTypes.h"

#include "Actions/SpellActionExecutorV3.h"   // oder der Header, der FSpellExecContextV3 wirklich definiert

#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"

void FDeliveryQueryHelpersV3::BuildQueryParams(
	const FSpellExecContextV3& Ctx,
	const FDeliveryQueryPolicyV3& Query,
	FCollisionQueryParams& OutParams)
{
	// Keep deterministic: do not use random or per-frame changing flags here.
	OutParams.bTraceComplex = false;

	if (Query.bIgnoreCaster)
	{
		if (AActor* Caster = Cast<AActor>(Ctx.GetCaster()))
		{
			OutParams.AddIgnoredActor(Caster);
		}
	}

	// If you later add IgnoreOwner / IgnoreTargets, apply them here once for all drivers.
}

FCollisionObjectQueryParams FDeliveryQueryHelpersV3::BuildObjectQueryParams(const FDeliveryQueryPolicyV3& Query)
{
	FCollisionObjectQueryParams Obj;

	// Assumes: Query.ObjectTypes is TArray<TEnumAsByte<ECollisionChannel>>
	// ADAPT HERE if you store EObjectTypeQuery instead.
	for (const TEnumAsByte<ECollisionChannel>& OT : Query.ObjectTypes)
	{
		Obj.AddObjectTypesToQuery((ECollisionChannel)OT.GetValue());
	}

	return Obj;
}

ECollisionChannel FDeliveryQueryHelpersV3::GetTraceChannel(const FDeliveryQueryPolicyV3& Query)
{
	// Assumes: TraceChannel is TEnumAsByte<ECollisionChannel> or similar wrapper
	return (ECollisionChannel)Query.TraceChannel.GetValue();
}

FName FDeliveryQueryHelpersV3::GetProfileNameOrDefault(const FDeliveryQueryPolicyV3& Query, const TCHAR* DefaultProfile)
{
	return (Query.CollisionProfile.Name != NAME_None)
		? Query.CollisionProfile.Name
		: FName(DefaultProfile);
}

bool FDeliveryQueryHelpersV3::OverlapMulti(
	UWorld* World,
	const FDeliveryQueryPolicyV3& Query,
	const FVector& Center,
	const FQuat& Rot,
	const FCollisionShape& Shape,
	const FCollisionQueryParams& Params,
	TArray<FOverlapResult>& OutOverlaps,
	const TCHAR* DefaultProfileForByProfile)
{
	if (!World) return false;

	bool bAny = false;

	switch (Query.FilterMode)
	{
	case EDeliveryQueryFilterModeV3::ByChannel:
	{
		const ECollisionChannel Ch = GetTraceChannel(Query);
		bAny = World->OverlapMultiByChannel(OutOverlaps, Center, Rot, Ch, Shape, Params);
		break;
	}
	case EDeliveryQueryFilterModeV3::ByObjectType:
	{
		const FCollisionObjectQueryParams ObjParams = BuildObjectQueryParams(Query);
		bAny = World->OverlapMultiByObjectType(OutOverlaps, Center, Rot, ObjParams, Shape, Params);
		break;
	}
	case EDeliveryQueryFilterModeV3::ByProfile:
	default:
	{
		const FName Profile = GetProfileNameOrDefault(Query, DefaultProfileForByProfile);
		bAny = World->OverlapMultiByProfile(OutOverlaps, Center, Rot, Profile, Shape, Params);
		break;
	}
	}

	if (bAny)
	{
		PostFilterByObjectTypes(Query, OutOverlaps);
	}

	return bAny;
}

bool FDeliveryQueryHelpersV3::SweepMulti(
	UWorld* World,
	const FDeliveryQueryPolicyV3& Query,
	const FVector& Start,
	const FVector& End,
	const FQuat& Rot,
	const FCollisionShape& Shape,
	const FCollisionQueryParams& Params,
	TArray<FHitResult>& OutHits,
	const TCHAR* DefaultProfileForByProfile)
{
	if (!World) return false;

	bool bAny = false;

	switch (Query.FilterMode)
	{
	case EDeliveryQueryFilterModeV3::ByChannel:
	{
		const ECollisionChannel Ch = GetTraceChannel(Query);
		bAny = World->SweepMultiByChannel(OutHits, Start, End, Rot, Ch, Shape, Params);
		break;
	}
	case EDeliveryQueryFilterModeV3::ByObjectType:
	{
		const FCollisionObjectQueryParams ObjParams = BuildObjectQueryParams(Query);
		bAny = World->SweepMultiByObjectType(OutHits, Start, End, Rot, ObjParams, Shape, Params);
		break;
	}
	case EDeliveryQueryFilterModeV3::ByProfile:
	default:
	{
		const FName Profile = GetProfileNameOrDefault(Query, DefaultProfileForByProfile);
		bAny = World->SweepMultiByProfile(OutHits, Start, End, Rot, Profile, Shape, Params);
		break;
	}
	}

	if (bAny)
	{
		PostFilterByObjectTypes(Query, OutHits);
	}

	return bAny;
}

bool FDeliveryQueryHelpersV3::LineTraceMulti(
	UWorld* World,
	const FDeliveryQueryPolicyV3& Query,
	const FVector& Start,
	const FVector& End,
	const FCollisionQueryParams& Params,
	TArray<FHitResult>& OutHits,
	const TCHAR* DefaultProfileForByProfile)
{
	if (!World) return false;

	bool bAny = false;

	switch (Query.FilterMode)
	{
	case EDeliveryQueryFilterModeV3::ByChannel:
	{
		const ECollisionChannel Ch = GetTraceChannel(Query);
		bAny = World->LineTraceMultiByChannel(OutHits, Start, End, Ch, Params);
		break;
	}
	case EDeliveryQueryFilterModeV3::ByObjectType:
	{
		const FCollisionObjectQueryParams ObjParams = BuildObjectQueryParams(Query);
		bAny = World->LineTraceMultiByObjectType(OutHits, Start, End, ObjParams, Params);
		break;
	}
	case EDeliveryQueryFilterModeV3::ByProfile:
	default:
	{
		const FName Profile = GetProfileNameOrDefault(Query, DefaultProfileForByProfile);
		bAny = World->LineTraceMultiByProfile(OutHits, Start, End, Profile, Params);
		break;
	}
	}

	if (bAny)
	{
		PostFilterByObjectTypes(Query, OutHits);
	}

	return bAny;
}

void FDeliveryQueryHelpersV3::PostFilterByObjectTypes(const FDeliveryQueryPolicyV3& Query, TArray<FOverlapResult>& InOutOverlaps)
{
	if (Query.FilterMode != EDeliveryQueryFilterModeV3::ByObjectType) return;
	if (Query.ObjectTypes.Num() == 0) return;

	InOutOverlaps.RemoveAll([&](const FOverlapResult& O)
	{
		const UPrimitiveComponent* Comp = O.GetComponent();
		if (!Comp) return true;

		const ECollisionChannel ObjType = Comp->GetCollisionObjectType();
		for (const TEnumAsByte<ECollisionChannel>& OT : Query.ObjectTypes)
		{
			if ((ECollisionChannel)OT.GetValue() == ObjType)
			{
				return false; // keep
			}
		}
		return true; // drop
	});
}

void FDeliveryQueryHelpersV3::PostFilterByObjectTypes(const FDeliveryQueryPolicyV3& Query, TArray<FHitResult>& InOutHits)
{
	if (Query.FilterMode != EDeliveryQueryFilterModeV3::ByObjectType) return;
	if (Query.ObjectTypes.Num() == 0) return;

	InOutHits.RemoveAll([&](const FHitResult& H)
	{
		const UPrimitiveComponent* Comp = H.GetComponent();
		if (!Comp) return true;

		const ECollisionChannel ObjType = Comp->GetCollisionObjectType();
		for (const TEnumAsByte<ECollisionChannel>& OT : Query.ObjectTypes)
		{
			if ((ECollisionChannel)OT.GetValue() == ObjType)
			{
				return false; // keep
			}
		}
		return true; // drop
	});
}
