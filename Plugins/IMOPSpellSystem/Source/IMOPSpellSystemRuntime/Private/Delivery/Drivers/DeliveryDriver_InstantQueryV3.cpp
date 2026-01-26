#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "Actions/SpellActionExecutorV3.h"
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

void UDeliveryDriver_InstantQueryV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;

	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	const bool bAny = EvaluateOnce(Ctx, Group, PrimitiveCtx);

	// Instant query ends immediately
	Stop(Ctx, Group, bAny ? EDeliveryStopReasonV3::OnFirstHit : EDeliveryStopReasonV3::Expired);
}

bool UDeliveryDriver_InstantQueryV3::EvaluateOnce(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return false;
	}

	const FDeliveryPrimitiveSpecV3& Spec = PrimitiveCtx.Spec;
	const FDeliveryInstantQueryConfigV3& Q = Spec.InstantQuery;

	const FVector From = PrimitiveCtx.FinalPoseWS.GetLocation();
	const FVector Dir = PrimitiveCtx.FinalPoseWS.GetRotation().GetForwardVector().GetSafeNormal();
	const float Range = FMath::Max(0.f, Q.Range);
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

	// Mode selection: Ray => LineTrace, otherwise Sweep/Overlap depending on spec
	if (Spec.Shape.Kind == EDeliveryShapeV3::Ray || Spec.Query.Mode == EDeliveryQueryModeV3::LineTrace)
	{
		bAny = Q.bMultiHit
			? World->LineTraceMultiByProfile(Hits, From, To, Profile, Params)
			: World->LineTraceSingleByProfile(Hits.AddDefaulted_GetRef(), From, To, Profile, Params);
	}
	else
	{
		FCollisionShape CS;
		switch (Spec.Shape.Kind)
		{
			case EDeliveryShapeV3::Sphere:  CS = FCollisionShape::MakeSphere(FMath::Max(0.f, Spec.Shape.Radius)); break;
			case EDeliveryShapeV3::Capsule: CS = FCollisionShape::MakeCapsule(FMath::Max(0.f, Spec.Shape.Radius), FMath::Max(0.f, Spec.Shape.HalfHeight)); break;
			case EDeliveryShapeV3::Box:     CS = FCollisionShape::MakeBox(Spec.Shape.Extents); break;
			default:                        CS = FCollisionShape::MakeSphere(0.f); break;
		}

		// Sweep along the line; if author asked Overlap, do a stationary overlap at From.
		if (Spec.Query.Mode == EDeliveryQueryModeV3::Overlap)
		{
			bAny = World->OverlapMultiByProfile(Hits, From, FQuat::Identity, Profile, CS, Params);
		}
		else
		{
			bAny = World->SweepMultiByProfile(Hits, From, To, FQuat::Identity, Profile, CS, Params);
		}
	}

	if (bAny)
	{
		SortHitsDeterministic(Hits, From);
	}

	// Cap hits
	const int32 MaxHits = FMath::Max(1, Q.MaxHits);
	if (Hits.Num() > MaxHits)
	{
		Hits.SetNum(MaxHits);
	}

	// Debug draw
	const FDeliveryDebugDrawConfigV3 DebugCfg =
		(Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

	if (DebugCfg.bEnable)
	{
		const float Dur = DebugCfg.Duration;

		if (DebugCfg.bDrawPath)
		{
			DrawDebugLine(World, From, To, FColor::Cyan, false, Dur, 0, 2.0f);
			DrawDebugDirectionalArrow(World, From, From + Dir * 120.f, 25.f, FColor::Cyan, false, Dur, 0, 2.0f);
		}

		if (DebugCfg.bDrawHits)
		{
			for (const FHitResult& H : Hits)
			{
				DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, Dur);
			}
		}
	}

	// Write hits to TargetStore (optional)
	const FName OutSetName = ResolveOutTargetSetName(Group, PrimitiveCtx);
	if (OutSetName != NAME_None)
	{
		if (USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get()))
		{
			FTargetSetV3 Out;
			for (const FHitResult& H : Hits)
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

	UE_LOG(LogIMOPDeliveryInstantQueryV3, Verbose, TEXT("InstantQuery: %s/%s hits=%d any=%d"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		Hits.Num(),
		bAny ? 1 : 0);

	return bAny;
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
