#include "Delivery/Drivers/DeliveryDriver_BeamV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "Actions/SpellActionExecutorV3.h" // FSpellExecContextV3 helpers
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

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
		if (DA != DB)
		{
			return DA < DB;
		}
		const FName NA = A.GetActor() ? A.GetActor()->GetFName() : NAME_None;
		const FName NB = B.GetActor() ? B.GetActor()->GetFName() : NAME_None;
		return NA.LexicalLess(NB);
	});
}

bool UDeliveryDriver_BeamV3::TryResolveLockTargetLocation(const FSpellExecContextV3& Ctx, const FDeliveryBeamConfigV3& BeamCfg, FVector& OutTargetLoc) const
{
	if (!BeamCfg.bLockOnTarget || BeamCfg.LockTargetSet == NAME_None)
	{
		return false;
	}

	USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get());
	if (!Store)
	{
		return false;
	}

	const FTargetSetV3* TS = Store->Find(BeamCfg.LockTargetSet);
	if (!TS || TS->Targets.Num() == 0)
	{
		return false;
	}

	// Deterministic: pick first target (store should already be deterministic)
	const FTargetRefV3& R = TS->Targets[0];
	AActor* A = R.Actor.Get();
	if (!A)
	{
		return false;
	}

	OutTargetLoc = A->GetActorLocation();
	return true;
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
		UE_LOG(LogIMOPDeliveryBeamV3, Error, TEXT("Beam Start failed: World is null"));
		Stop(Ctx, Group, EDeliveryStopReasonV3::Failed);
		return;
	}

	// Schedule first eval immediately
	NextEvalTimeSeconds = World->GetTimeSeconds();

	// Optional immediate eval
	EvaluateOnce(Ctx, Group, PrimitiveCtx);

	UE_LOG(LogIMOPDeliveryBeamV3, Log, TEXT("Beam started: %s/%s"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString());
}

void UDeliveryDriver_BeamV3::Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds)
{
	if (!bActive)
	{
		return;
	}

	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return;
	}

	// Use live ctx (rig updates)
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
	const FDeliveryBeamConfigV3& BeamCfg = Spec.Beam;

	const FVector From = PrimitiveCtx.FinalPoseWS.GetLocation();

	// Aim direction:
	FVector Dir = PrimitiveCtx.FinalPoseWS.GetRotation().GetForwardVector().GetSafeNormal();
	FVector LockLoc;
	if (TryResolveLockTargetLocation(Ctx, BeamCfg, LockLoc))
	{
		Dir = (LockLoc - From).GetSafeNormal();
	}

	const float Range = FMath::Max(0.f, BeamCfg.Range);
	const FVector To = From + Dir * Range;

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

	const float Radius = FMath::Max(0.f, BeamCfg.Radius);
	if (Radius <= 0.f)
	{
		bAny = World->LineTraceMultiByProfile(Hits, From, To, Profile, Params);
	}
	else
	{
		const FCollisionShape CS = FCollisionShape::MakeSphere(Radius);
		bAny = World->SweepMultiByProfile(Hits, From, To, FQuat::Identity, Profile, CS, Params);
	}

	if (bAny)
	{
		SortHitsDeterministic(Hits, From);
	}

	// Debug draw
	const FDeliveryDebugDrawConfigV3 DebugCfg =
		(Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

	if (DebugCfg.bEnable)
	{
		const float Dur = DebugCfg.Duration;

		if (DebugCfg.bDrawPath)
		{
			DrawDebugLine(World, From, To, FColor::Blue, false, Dur, 0, 2.0f);
			DrawDebugDirectionalArrow(World, From, From + Dir * 120.f, 25.f, FColor::Blue, false, Dur, 0, 2.0f);
		}

		if (DebugCfg.bDrawHits)
		{
			const int32 MaxDraw = FMath::Min(32, Hits.Num());
			for (int32 i = 0; i < MaxDraw; ++i)
			{
				const FHitResult& H = Hits[i];
				DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, Dur);
				DrawDebugLine(World, H.ImpactPoint, H.ImpactPoint + H.ImpactNormal * 35.f, FColor::Yellow, false, Dur, 0, 1.2f);
			}
		}

		// Visualize beam radius
		if (Radius > 0.f && DebugCfg.bDrawShape)
		{
			DrawDebugSphere(World, From, Radius, 12, FColor::Blue, false, Dur, 0, 0.5f);
			DrawDebugSphere(World, To, Radius, 12, FColor::Blue, false, Dur, 0, 0.5f);
		}
	}

	// Optional writeback into TargetStore
	const FName OutSetName = ResolveOutTargetSetName(Group, PrimitiveCtx);
	if (OutSetName != NAME_None)
	{
		USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get());
		if (Store)
		{
			FTargetSetV3 OutSet;
			OutSet.Targets.Reserve(Hits.Num());

			for (const FHitResult& H : Hits)
			{
				AActor* A = H.GetActor();
				if (!A)
				{
					continue;
				}
				FTargetRefV3 R;
				R.Actor = A;
				OutSet.AddUnique(R);
			}

			Store->Set(OutSetName, OutSet);
		}
	}

	UE_LOG(LogIMOPDeliveryBeamV3, Verbose, TEXT("Beam tick: %s/%s hits=%d radius=%.2f"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		Hits.Num(),
		Radius);

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
