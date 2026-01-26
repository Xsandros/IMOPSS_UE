#include "Delivery/Drivers/DeliveryDriver_MoverV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionShape.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryMoverV3, Log, All);

FName UDeliveryDriver_MoverV3::ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
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

bool UDeliveryDriver_MoverV3::TryResolveHomingTargetLocation(const FSpellExecContextV3& Ctx, const FDeliveryMoverConfigV3& MoverCfg, FVector& OutTargetLoc) const
{
	if (MoverCfg.Motion != EMoverMotionKindV3::Homing || MoverCfg.HomingTargetSet == NAME_None)
	{
		return false;
	}

	USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get());
	if (!Store)
	{
		return false;
	}

	const FTargetSetV3* TS = Store->Find(MoverCfg.HomingTargetSet);
	if (!TS || TS->Targets.Num() == 0)
	{
		return false;
	}

	const FTargetRefV3& R = TS->Targets[0];
	AActor* A = R.Actor.Get();
	if (!A)
	{
		return false;
	}

	OutTargetLoc = A->GetActorLocation();
	return true;
}

void UDeliveryDriver_MoverV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;

	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPDeliveryMoverV3, Error, TEXT("Mover Start failed: World is null"));
		Stop(Ctx, Group, EDeliveryStopReasonV3::Failed);
		return;
	}

	// Initialize pos/vel from spawn pose
	PosWS = PrimitiveCtx.FinalPoseWS.GetLocation();

	const FVector Fwd = PrimitiveCtx.FinalPoseWS.GetRotation().GetForwardVector().GetSafeNormal();
	const float Speed = FMath::Max(0.f, PrimitiveCtx.Spec.Mover.Speed);
	VelWS = Fwd * Speed;

	DistanceTraveled = 0.f;
	PierceCount = 0;

	NextSimTimeSeconds = World->GetTimeSeconds(); // immediate

	UE_LOG(LogIMOPDeliveryMoverV3, Log, TEXT("Mover started: %s/%s pos=%s speed=%.1f"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		*PosWS.ToString(),
		Speed);
}

void UDeliveryDriver_MoverV3::Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds)
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

	// Use live ctx for config updates (rig updates don't affect projectile after spawn, but config may)
	const FDeliveryContextV3* LiveCtx = Group->PrimitiveCtxById.Find(PrimitiveId);
	const FDeliveryContextV3& PCtx = LiveCtx ? *LiveCtx : LocalCtx;
	const FDeliveryMoverConfigV3& M = PCtx.Spec.Mover;

	const float Now = World->GetTimeSeconds();
	const float Interval = FMath::Max(0.f, M.TickInterval);

	if (Interval > 0.f && Now < NextSimTimeSeconds)
	{
		return;
	}

	// Stable schedule
	NextSimTimeSeconds = (Interval > 0.f) ? (Now + Interval) : Now;

	const float StepDt = (Interval > 0.f) ? Interval : DeltaSeconds;

	StepSim(Ctx, Group, StepDt);

	// Max distance stop
	if (M.MaxDistance > 0.f && DistanceTraveled >= M.MaxDistance)
	{
		Stop(Ctx, Group, EDeliveryStopReasonV3::Expired);
	}
}

bool UDeliveryDriver_MoverV3::StepSim(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float StepDt)
{
	if (!bActive || StepDt <= 0.f)
	{
		return false;
	}

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		return false;
	}

	// Pull config from live ctx
	const FDeliveryContextV3* LiveCtx = Group ? Group->PrimitiveCtxById.Find(PrimitiveId) : nullptr;
	const FDeliveryContextV3& PCtx = LiveCtx ? *LiveCtx : LocalCtx;
	const FDeliveryMoverConfigV3& M = PCtx.Spec.Mover;

	// Homing adjust
	if (M.Motion == EMoverMotionKindV3::Homing)
	{
		FVector TargetLoc;
		if (TryResolveHomingTargetLocation(Ctx, M, TargetLoc))
		{
			const FVector ToTarget = (TargetLoc - PosWS).GetSafeNormal();
			const float Strength = FMath::Max(0.f, M.HomingStrength);

			// Simple deterministic blend (no physics)
			const FVector DesiredVel = ToTarget * FMath::Max(0.f, M.Speed);
			VelWS = FMath::Lerp(VelWS, DesiredVel, FMath::Clamp(Strength * StepDt, 0.f, 1.f));
		}
	}

	// Gravity (ballistic)
	if (M.Motion == EMoverMotionKindV3::Ballistic && M.GravityScale != 0.f)
	{
		const FVector G = FVector(0,0, World->GetGravityZ()) * M.GravityScale;
		VelWS += G * StepDt;
	}

	const FVector From = PosWS;
	const FVector To = PosWS + VelWS * StepDt;

	// Debug cfg
	const FDeliveryDebugDrawConfigV3 DebugCfg =
		(PCtx.Spec.bOverrideDebugDraw ? PCtx.Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

	if (DebugCfg.bEnable && DebugCfg.bDrawPath)
	{
		DebugDrawStep(World, DebugCfg, From, To);
	}

	// Hit test (sweep along segment)
	TArray<FHitResult> Hits;
	const bool bHitAny = DoSweepHit(Ctx, Group, From, To, Hits);

	if (!bHitAny)
	{
		PosWS = To;
		DistanceTraveled += FVector::Dist(From, To);
		return false;
	}

	// Sort by distance
	Hits.Sort([&](const FHitResult& A, const FHitResult& B)
	{
		return A.Distance < B.Distance;
	});

	// Handle hits
	const bool bStopOnHit = M.bStopOnHit;
	const bool bPierce = M.bPierce;
	const int32 MaxPierce = FMath::Max(0, M.MaxPierceHits);

	// Optional: write first hit target set
	const FName OutSetName = ResolveOutTargetSetName(Group, PCtx);

	if (DebugCfg.bEnable && DebugCfg.bDrawHits)
	{
		for (const FHitResult& H : Hits)
		{
			DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, DebugCfg.Duration);
			DrawDebugLine(World, H.ImpactPoint, H.ImpactPoint + H.ImpactNormal * 35.f, FColor::Yellow, false, DebugCfg.Duration, 0, 1.2f);
		}
	}

	if (OutSetName != NAME_None)
	{
		USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get());
		if (Store)
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

	// Move to first impact point
	const FHitResult& First = Hits[0];
	PosWS = First.ImpactPoint;
	DistanceTraveled += FVector::Dist(From, PosWS);

	// Decide stop/pierce
	if (bPierce)
	{
		PierceCount++;
		if (MaxPierce > 0 && PierceCount > MaxPierce)
		{
			Stop(Ctx, Group, EDeliveryStopReasonV3::OnFirstHit);
			return true;
		}

		// Continue after pierce: keep velocity, but nudge forward a bit to avoid re-hitting same surface.
		PosWS += VelWS.GetSafeNormal() * 2.f;
		return true;
	}

	if (bStopOnHit)
	{
		Stop(Ctx, Group, EDeliveryStopReasonV3::OnFirstHit);
		return true;
	}

	// Otherwise continue (rare config)
	PosWS = To;
	DistanceTraveled += FVector::Dist(First.ImpactPoint, To);
	return true;
}

bool UDeliveryDriver_MoverV3::DoSweepHit(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FVector& From, const FVector& To, TArray<FHitResult>& OutHits) const
{
	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return false;
	}

	const FDeliveryContextV3* LiveCtx = Group->PrimitiveCtxById.Find(PrimitiveId);
	const FDeliveryContextV3& PCtx = LiveCtx ? *LiveCtx : LocalCtx;

	const FDeliveryPrimitiveSpecV3& Spec = PCtx.Spec;
	const FName Profile = (Spec.Query.CollisionProfile != NAME_None) ? Spec.Query.CollisionProfile : FName("Visibility");

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Mover), /*bTraceComplex*/ false);
	if (Spec.Query.bIgnoreCaster)
	{
		if (AActor* Caster = Ctx.GetCaster())
		{
			Params.AddIgnoredActor(Caster);
		}
	}

	// Use shape kind as sweep volume (Ray -> line trace)
	if (Spec.Shape.Kind == EDeliveryShapeV3::Ray)
	{
		return World->LineTraceMultiByProfile(OutHits, From, To, Profile, Params);
	}

	FCollisionShape CS;
	switch (Spec.Shape.Kind)
	{
		case EDeliveryShapeV3::Sphere:   CS = FCollisionShape::MakeSphere(FMath::Max(0.f, Spec.Shape.Radius)); break;
		case EDeliveryShapeV3::Capsule:  CS = FCollisionShape::MakeCapsule(FMath::Max(0.f, Spec.Shape.Radius), FMath::Max(0.f, Spec.Shape.HalfHeight)); break;
		case EDeliveryShapeV3::Box:      CS = FCollisionShape::MakeBox(Spec.Shape.Extents); break;
		default:                         CS = FCollisionShape::MakeSphere(0.f); break;
	}

	return World->SweepMultiByProfile(OutHits, From, To, FQuat::Identity, Profile, CS, Params);
}

void UDeliveryDriver_MoverV3::DebugDrawStep(UWorld* World, const FDeliveryDebugDrawConfigV3& DebugCfg, const FVector& From, const FVector& To) const
{
	if (!World)
	{
		return;
	}

	DrawDebugLine(World, From, To, FColor::Purple, false, DebugCfg.Duration, 0, 2.0f);
	DrawDebugPoint(World, To, 8.f, FColor::Purple, false, DebugCfg.Duration);
}

void UDeliveryDriver_MoverV3::Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}

	bActive = false;

	UE_LOG(LogIMOPDeliveryMoverV3, Log, TEXT("Mover stopped: %s/%s inst=%d reason=%d pos=%s dist=%.1f"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		GroupHandle.InstanceIndex,
		(int32)Reason,
		*PosWS.ToString(),
		DistanceTraveled);
}
