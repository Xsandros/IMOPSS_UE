#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionShape.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryInstantQueryV3, Log, All);

FName UDeliveryDriver_InstantQueryV3::ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	if (PrimitiveCtx.Spec.OutTargetSetName != NAME_None)
	{
		return PrimitiveCtx.Spec.OutTargetSetName;
	}
	if (Group && Group->GroupSpec.OutTargetSetDefault != NAME_None)
	{
		return Group->GroupSpec.OutTargetSetDefault;
	}
	return NAME_None;
}

void UDeliveryDriver_InstantQueryV3::SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From)
{
	Hits.Sort([&](const FHitResult& A, const FHitResult& B)
	{
		const float DA = FVector::DistSquared(From, A.ImpactPoint);
		const float DB = FVector::DistSquared(From, B.ImpactPoint);
		if (DA != DB) return DA < DB;

		const FName NA = A.GetActor() ? A.GetActor()->GetFName() : NAME_None;
		const FName NB = B.GetActor() ? B.GetActor()->GetFName() : NAME_None;
		return NA.LexicalLess(NB);
	});
}

static FCollisionShape MakeShape(const FDeliveryShapeV3& S)
{
	switch (S.Kind)
	{
		case EDeliveryShapeV3::Sphere:  return FCollisionShape::MakeSphere(FMath::Max(0.f, S.Radius));
		case EDeliveryShapeV3::Capsule: return FCollisionShape::MakeCapsule(FMath::Max(0.f, S.Radius), FMath::Max(0.f, S.HalfHeight));
		case EDeliveryShapeV3::Box:     return FCollisionShape::MakeBox(S.Extents);
		case EDeliveryShapeV3::Ray:
		default:
			return FCollisionShape::MakeSphere(0.f);
	}
}

void UDeliveryDriver_InstantQueryV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;

	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	if (!Ctx.GetWorld() || !Group)
	{
		UE_LOG(LogIMOPDeliveryInstantQueryV3, Error, TEXT("InstantQuery Start failed: World/Group missing"));
		Stop(Ctx, Group, EDeliveryStopReasonV3::Failed);
		return;
	}

	EvaluateOnce(Ctx, Group, PrimitiveCtx);

	// InstantQuery ends immediately
	Stop(Ctx, Group, EDeliveryStopReasonV3::OnFirstHit);
}

bool UDeliveryDriver_InstantQueryV3::EvaluateOnce(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return false;
	}

	const FDeliveryPrimitiveSpecV3& Spec = PrimitiveCtx.Spec;

	const FVector From = PrimitiveCtx.FinalPoseWS.GetLocation();
	const FVector Dir = PrimitiveCtx.FinalPoseWS.GetRotation().GetForwardVector().GetSafeNormal();
	const float Range = FMath::Max(0.f, Spec.InstantQuery.Range);
	const FVector To = From + Dir * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_InstantQuery), /*bTraceComplex*/ false);
	if (Spec.Query.bIgnoreCaster)
	{
		if (AActor* Caster = Ctx.GetCaster())
		{
			Params.AddIgnoredActor(Caster);
		}
	}

	const FName Profile = (Spec.Query.CollisionProfile != NAME_None) ? Spec.Query.CollisionProfile : FName("Visibility");

	TArray<FHitResult> Hits;
	bool bAny = false;

	// Ray/LineTrace => trace; otherwise sweep/overlap
	const bool bRay = (Spec.Shape.Kind == EDeliveryShapeV3::Ray) || (Spec.Query.Mode == EDeliveryQueryModeV3::LineTrace);

	if (bRay)
	{
		bAny = World->LineTraceMultiByProfile(Hits, From, To, Profile, Params);
	}
	else if (Spec.Query.Mode == EDeliveryQueryModeV3::Overlap)
	{
		const FCollisionShape CS = MakeShape(Spec.Shape);
		bAny = World->OverlapMultiByProfile(Hits, To, FQuat::Identity, Profile, CS, Params);
	}
	else
	{
		const FCollisionShape CS = MakeShape(Spec.Shape);
		bAny = World->SweepMultiByProfile(Hits, From, To, FQuat::Identity, Profile, CS, Params);
	}

	if (bAny)
	{
		SortHitsDeterministic(Hits, From);
	}

	// Clamp hits
	const int32 MaxHits = FMath::Max(1, Spec.InstantQuery.MaxHits);
	const bool bMulti = Spec.InstantQuery.bMultiHit;

	TArray<FHitResult> Final;
	Final.Reserve(FMath::Min(MaxHits, Hits.Num()));
	for (const FHitResult& H : Hits)
	{
		if (!H.GetActor())
		{
			continue;
		}
		Final.Add(H);
		if (!bMulti || Final.Num() >= MaxHits)
		{
			break;
		}
	}

	// Debug
	const FDeliveryDebugDrawConfigV3 DebugCfg =
		(Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

	if (DebugCfg.bEnable)
	{
		const float Dur = DebugCfg.Duration;
		if (DebugCfg.bDrawPath)
		{
			DrawDebugLine(World, From, To, FColor::Cyan, false, Dur, 0, 1.5f);
			DrawDebugDirectionalArrow(World, From, From + Dir * 120.f, 25.f, FColor::Cyan, false, Dur, 0, 1.5f);
		}
		if (DebugCfg.bDrawHits)
		{
			for (const FHitResult& H : Final)
			{
				DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, Dur);
				DrawDebugLine(World, H.ImpactPoint, H.ImpactPoint + H.ImpactNormal * 30.f, FColor::Yellow, false, Dur, 0, 1.0f);
			}
		}
	}

	// Write back to TargetStore
	const FName OutSetName = ResolveOutTargetSetName(Group, PrimitiveCtx);
	if (OutSetName != NAME_None)
	{
		if (USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get()))
		{
			FTargetSetV3 Out;
			for (const FHitResult& H : Final)
			{
				if (AActor* A = H.GetActor())
				{
					FTargetRefV3 R;
					R.Actor = A;
					Out.AddUnique(R);
				}
			}
			Store->Set(OutSetName, Out);
		}
	}

	return Final.Num() > 0;
}

void UDeliveryDriver_InstantQueryV3::Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}

	bActive = false;

	UE_LOG(LogIMOPDeliveryInstantQueryV3, Log, TEXT("InstantQuery stopped: %s/%s inst=%d reason=%d"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		GroupHandle.InstanceIndex,
		(int32)Reason);
}
