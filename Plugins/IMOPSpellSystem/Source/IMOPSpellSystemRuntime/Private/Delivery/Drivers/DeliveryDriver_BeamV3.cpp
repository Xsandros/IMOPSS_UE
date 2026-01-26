#include "Delivery/Drivers/DeliveryDriver_BeamV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"
#include "Stores/SpellTargetStoreV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionShape.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryBeamV3, Log, All);

FName UDeliveryDriver_BeamV3::ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
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

void UDeliveryDriver_BeamV3::SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From)
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

void UDeliveryDriver_BeamV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;

	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPDeliveryBeamV3, Error, TEXT("Beam Start failed: World null"));
		Stop(Ctx, Group, EDeliveryStopReasonV3::Failed);
		return;
	}

	NextEvalTimeSeconds = World->GetTimeSeconds();

	UE_LOG(LogIMOPDeliveryBeamV3, Log, TEXT("Beam started: %s/%s"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString());
}

bool UDeliveryDriver_BeamV3::ComputeBeamEndpoints(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx, FVector& OutFrom, FVector& OutTo) const
{
	const FDeliveryPrimitiveSpecV3& Spec = PrimitiveCtx.Spec;
	const FDeliveryBeamConfigV3& B = Spec.Beam;

	OutFrom = PrimitiveCtx.FinalPoseWS.GetLocation();

	// Default direction: forward from pose
	FVector Dir = PrimitiveCtx.FinalPoseWS.GetRotation().GetForwardVector().GetSafeNormal();
	float Range = FMath::Max(0.f, B.Range);

	// Optional lock-on: use first actor in LockTargetSet if present
	if (B.bLockOnTarget && B.LockTargetSet != NAME_None)
	{
		if (USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get()))
		{
			if (const FTargetSetV3* TS = Store->Get(B.LockTargetSet))
			{
				for (const FTargetRefV3& R : TS->Targets)
				{
					if (AActor* A = R.Actor.Get())
					{
						const FVector ToActor = (A->GetActorLocation() - OutFrom);
						if (!ToActor.IsNearlyZero())
						{
							Dir = ToActor.GetSafeNormal();
						}
						break;
					}
				}
			}
		}
	}

	OutTo = OutFrom + Dir * Range;
	return true;
}

void UDeliveryDriver_BeamV3::Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds)
{
	if (!bActive || !Group)
	{
		return;
	}

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		return;
	}

	// Live ctx (after rig updates)
	const FDeliveryContextV3* LiveCtx = Group->PrimitiveCtxById.Find(PrimitiveId);
	const FDeliveryContextV3& PCtx = LiveCtx ? *LiveCtx : LocalCtx;

	const float Now = World->GetTimeSeconds();
	const float Interval = FMath::Max(0.f, PCtx.Spec.Beam.TickInterval);

	if (Interval > 0.f && Now < NextEvalTimeSeconds)
	{
		return;
	}

	NextEvalTimeSeconds = (Interval > 0.f) ? (Now + Interval) : Now;

	EvaluateOnce(Ctx, Group, PCtx);
}

bool UDeliveryDriver_BeamV3::EvaluateOnce(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return false;
	}

	const FDeliveryPrimitiveSpecV3& Spec = PrimitiveCtx.Spec;
	const FDeliveryBeamConfigV3& B = Spec.Beam;

	FVector From, To;
	ComputeBeamEndpoints(Ctx, Group, PrimitiveCtx, From, To);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Beam), /*bTraceComplex*/ false);
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

	const float Radius = FMath::Max(0.f, B.Radius);

	if (Radius <= KINDA_SMALL_NUMBER)
	{
		// Line trace (single/multi depends on query mode)
		if (Spec.Query.Mode == EDeliveryQueryModeV3::LineTrace || Spec.Shape.Kind == EDeliveryShapeV3::Ray)
		{
			bAny = World->LineTraceMultiByProfile(Hits, From, To, Profile, Params);
		}
		else
		{
			// Still do a line trace for beams unless explicitly wanting overlap.
			bAny = World->LineTraceMultiByProfile(Hits, From, To, Profile, Params);
		}
	}
	else
	{
		FCollisionShape CS = FCollisionShape::MakeSphere(Radius);
		bAny = World->SweepMultiByProfile(Hits, From, To, FQuat::Identity, Profile, CS, Params);
	}

	if (bAny)
	{
		SortHitsDeterministic(Hits, From);
	}

	// Debug
	const FDeliveryDebugDrawConfigV3 DebugCfg =
		(Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

	if (DebugCfg.bEnable)
	{
		const float Dur = DebugCfg.Duration;

		if (DebugCfg.bDrawPath)
		{
			DrawDebugLine(World, From, To, FColor::Blue, false, Dur, 0, 2.0f);
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

	// Stop policy: if group/primitive wants stop on first hit, subsystem will stop us via StopByPrimitiveId later.
	// For now keep beam running.

	return bAny;
}

void UDeliveryDriver_BeamV3::Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}

	bActive = false;

	UE_LOG(LogIMOPDeliveryBeamV3, Log, TEXT("Beam stopped: %s/%s inst=%d reason=%d"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		GroupHandle.InstanceIndex,
		(int32)Reason);
}
