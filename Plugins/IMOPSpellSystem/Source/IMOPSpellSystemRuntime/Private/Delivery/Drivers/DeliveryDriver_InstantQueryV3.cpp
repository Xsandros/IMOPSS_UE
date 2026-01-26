#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionShape.h"

// Optional writeback (if you have these, it will work; otherwise comment out includes + blocks)
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

// If your Ctx provides TargetStore / GetWorld, this will compile.
// If not, adjust after first compile error.
#include "Actions/SpellActionExecutorV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryInstantQueryV3, Log, All);

static FCollisionShape MakeCollisionShape(const FDeliveryShapeV3& Shape)
{
	switch (Shape.Kind)
	{
		case EDeliveryShapeV3::Sphere:
			return FCollisionShape::MakeSphere(FMath::Max(0.f, Shape.Radius));

		case EDeliveryShapeV3::Capsule:
			return FCollisionShape::MakeCapsule(FMath::Max(0.f, Shape.Radius), FMath::Max(0.f, Shape.HalfHeight));

		case EDeliveryShapeV3::Box:
			return FCollisionShape::MakeBox(Shape.Extents);

		case EDeliveryShapeV3::Ray:
		default:
			// Ray is handled by line trace; fallback to sphere(0)
			return FCollisionShape::MakeSphere(0.f);
	}
}

void UDeliveryDriver_InstantQueryV3::SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From)
{
	Hits.Sort([&](const FHitResult& A, const FHitResult& B)
	{
		const float DA = FVector::DistSquared(From, A.ImpactPoint);
		const float DB = FVector::DistSquared(From, B.ImpactPoint);
		if (DA != DB)
		{
			return DA < DB;
		}

		// Use Actor name as stable-ish secondary key (server authoritative)
		const FName NA = A.GetActor() ? A.GetActor()->GetFName() : NAME_None;
		const FName NB = B.GetActor() ? B.GetActor()->GetFName() : NAME_None;
		return NA.LexicalLess(NB);
	});
}

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

bool UDeliveryDriver_InstantQueryV3::DoOverlapQuery(
	UWorld* World,
	const FVector& Origin,
	const FQuat& Rot,
	const FDeliveryShapeV3& Shape,
	const FName& Profile,
	const FCollisionQueryParams& Params,
	TArray<FHitResult>& OutHits) const
{
	if (!World)
	{
		return false;
	}

	const FCollisionShape CS = MakeCollisionShape(Shape);

	// For Ray, overlap doesn't make sense; treat as point overlap using a tiny sphere.
	if (Shape.Kind == EDeliveryShapeV3::Ray)
	{
		const FCollisionShape Tiny = FCollisionShape::MakeSphere(1.f);
		return World->OverlapMultiByProfile(OutHits, Origin, Rot, Profile, Tiny, Params);
	}

	return World->OverlapMultiByProfile(OutHits, Origin, Rot, Profile, CS, Params);
}

bool UDeliveryDriver_InstantQueryV3::DoSweepQuery(
	UWorld* World,
	const FVector& From,
	const FVector& To,
	const FQuat& Rot,
	const FDeliveryShapeV3& Shape,
	const FName& Profile,
	const FCollisionQueryParams& Params,
	TArray<FHitResult>& OutHits) const
{
	if (!World)
	{
		return false;
	}

	// Ray sweep -> line trace
	if (Shape.Kind == EDeliveryShapeV3::Ray)
	{
		return World->LineTraceMultiByProfile(OutHits, From, To, Profile, Params);
	}

	const FCollisionShape CS = MakeCollisionShape(Shape);
	return World->SweepMultiByProfile(OutHits, From, To, Rot, Profile, CS, Params);
}

bool UDeliveryDriver_InstantQueryV3::DoLineTraceQuery(
	UWorld* World,
	const FVector& From,
	const FVector& To,
	const FName& Profile,
	const FCollisionQueryParams& Params,
	TArray<FHitResult>& OutHits) const
{
	if (!World)
	{
		return false;
	}

	return World->LineTraceMultiByProfile(OutHits, From, To, Profile, Params);
}

void UDeliveryDriver_InstantQueryV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;

	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPDeliveryInstantQueryV3, Error, TEXT("InstantQuery Start failed: World is null"));
		Stop(Ctx, Group, EDeliveryStopReasonV3::Failed);
		return;
	}

	const FDeliveryPrimitiveSpecV3& Spec = PrimitiveCtx.Spec;

	// Use final pose provided by subsystem (anchor + local offsets; motion will come later)
	const FVector From = PrimitiveCtx.FinalPoseWS.GetLocation();
	const FVector Dir = PrimitiveCtx.FinalPoseWS.GetRotation().GetForwardVector().GetSafeNormal();
	const float Range = FMath::Max(0.f, Spec.InstantQuery.Range);
	const FVector To = From + Dir * Range;

	// Build collision query params
	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_InstantQuery), /*bTraceComplex*/ false);
	if (Spec.Query.bIgnoreCaster && Ctx.Caster)
	{
		Params.AddIgnoredActor(Ctx.Caster);
	}

	const FName Profile = (Spec.Query.CollisionProfile != NAME_None) ? Spec.Query.CollisionProfile : FName("OverlapAll");

	// Debug draw
	const FDeliveryDebugDrawConfigV3 DebugCfg =
		(Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : (Group ? Group->GroupSpec.DebugDrawDefaults : FDeliveryDebugDrawConfigV3()));

	if (DebugCfg.bEnable && DebugCfg.bDrawPath)
	{
		DrawDebugLine(World, From, To, FColor::Cyan, false, DebugCfg.Duration, 0, 1.5f);
		DrawDebugDirectionalArrow(World, From, From + Dir * 120.f, 25.f, FColor::Cyan, false, DebugCfg.Duration, 0, 1.5f);
	}

	// Execute query
	TArray<FHitResult> Hits;
	bool bHitAny = false;

	switch (Spec.Query.Mode)
	{
		case EDeliveryQueryModeV3::Overlap:
			bHitAny = DoOverlapQuery(World, From, PrimitiveCtx.FinalPoseWS.GetRotation(), Spec.Shape, Profile, Params, Hits);
			break;

		case EDeliveryQueryModeV3::Sweep:
			bHitAny = DoSweepQuery(World, From, To, PrimitiveCtx.FinalPoseWS.GetRotation(), Spec.Shape, Profile, Params, Hits);
			break;

		case EDeliveryQueryModeV3::LineTrace:
		default:
			bHitAny = DoLineTraceQuery(World, From, To, Profile, Params, Hits);
			break;
	}

	if (bHitAny)
	{
		SortHitsDeterministic(Hits, From);
	}

	// Select final hits
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

	// Debug draw hits
	if (DebugCfg.bEnable && DebugCfg.bDrawHits)
	{
		for (const FHitResult& H : Final)
		{
			DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, DebugCfg.Duration);
			DrawDebugLine(World, H.ImpactPoint, H.ImpactPoint + H.ImpactNormal * 30.f, FColor::Yellow, false, DebugCfg.Duration, 0, 1.0f);
		}
	}

	// Optional: writeback into TargetStore (if your exec context provides it)
	const FName OutSetName = ResolveOutTargetSetName(Group, PrimitiveCtx);
	if (OutSetName != NAME_None && Ctx.TargetStore)
	{
		FTargetSetV3 OutSet;
		OutSet.Targets.Reserve(Final.Num());

		for (const FHitResult& H : Final)
		{
			if (AActor* A = H.GetActor())
			{
				FTargetRefV3 R;
				R.Actor = A;
				OutSet.AddUnique(R);
			}
		}

		Ctx.TargetStore->Set(OutSetName, OutSet);

		UE_LOG(LogIMOPDeliveryInstantQueryV3, Log, TEXT("InstantQuery Writeback: TargetSet=%s Count=%d"),
			*OutSetName.ToString(), OutSet.Targets.Num());
	}

	UE_LOG(LogIMOPDeliveryInstantQueryV3, Log, TEXT("InstantQuery: %s/%s hits=%d mode=%d shape=%d profile=%s"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		Final.Num(),
		(int32)Spec.Query.Mode,
		(int32)Spec.Shape.Kind,
		*Profile.ToString());

	// InstantQuery finishes immediately.
	Stop(Ctx, Group, (Final.Num() > 0) ? EDeliveryStopReasonV3::OnFirstHit : EDeliveryStopReasonV3::Manual);
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
