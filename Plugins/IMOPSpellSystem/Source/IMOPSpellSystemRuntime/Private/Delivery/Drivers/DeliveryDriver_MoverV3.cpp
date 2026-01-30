#include "Delivery/Drivers/DeliveryDriver_MoverV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "Delivery/Helpers/DeliveryQueryHelpersV3.h"
#include "Delivery/Helpers/DeliveryShapeHelpersV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionShape.h"

// Needed for FOverlapResult
#include "Engine/EngineTypes.h"

// Needed for FActorInstanceHandle used by FHitResult in UE5
#include "Engine/ActorInstanceHandle.h"
#include "Engine/OverlapResult.h"

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

void UDeliveryDriver_MoverV3::SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From)
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

FCollisionShape UDeliveryDriver_MoverV3::MakeCollisionShape(const FDeliveryShapeV3& Shape)
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
		// Ray handled via line trace; return tiny sphere for safety
		return FCollisionShape::MakeSphere(0.f);
	}
}

bool UDeliveryDriver_MoverV3::GetHomingTargetLocation(const FSpellExecContextV3& Ctx, const FDeliveryMoverConfigV3& M, FVector& OutLoc) const
{
	if (M.HomingTargetSet == NAME_None)
	{
		return false;
	}

	USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get());
	if (!Store)
	{
		return false;
	}

	FTargetSetV3 TS;
	if (!Store->Get(M.HomingTargetSet, TS))
	{
		return false;
	}

	for (const FTargetRefV3& R : TS.Targets)
	{
		if (AActor* A = R.Actor.Get())
		{
			OutLoc = A->GetActorLocation();
			return true;
		}
	}

	return false;
}

bool UDeliveryDriver_MoverV3::SweepMoveAndCollectHits(
	const FSpellExecContextV3& Ctx,
	UWorld* World,
	const FDeliveryPrimitiveSpecV3& Spec,
	const FVector& From,
	const FVector& To,
	TArray<FHitResult>& OutHits) const
{
	OutHits.Reset();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Mover), /*bTraceComplex*/ false);
	FDeliveryQueryHelpersV3::BuildQueryParams(Ctx, Spec.Query, Params);

	const bool bLine =
		(Spec.Shape.Kind == EDeliveryShapeV3::Ray) ||
		(Spec.Query.Mode == EDeliveryQueryModeV3::LineTrace);

	// Line traces
	if (bLine)
	{
		const bool bAny = FDeliveryQueryHelpersV3::LineTraceMulti(
			World,
			Spec.Query,
			From,
			To,
			Params,
			OutHits,
			TEXT("Visibility"));

		return bAny;
	}

	// Collision shapes (Sphere/Box/Capsule) for Sweep/Overlap
	FCollisionShape CS;
	const bool bHasCollisionShape = FDeliveryShapeHelpersV3::BuildCollisionShape(Spec.Shape, CS);
	if (!bHasCollisionShape)
	{
		return false;
	}

	// Overlap at destination
	if (Spec.Query.Mode == EDeliveryQueryModeV3::Overlap)
	{
		TArray<FOverlapResult> Overlaps;
		const bool bAny = FDeliveryQueryHelpersV3::OverlapMulti(
			World,
			Spec.Query,
			To,
			FQuat::Identity,
			CS,
			Params,
			Overlaps,
			TEXT("OverlapAllDynamic"));

		if (bAny)
		{
			OutHits.Reserve(Overlaps.Num());
			for (const FOverlapResult& O : Overlaps)
			{
				AActor* A = O.GetActor();
				if (!A) continue;

				FHitResult H;
				H.HitObjectHandle = FActorInstanceHandle(A);
				H.Component = O.GetComponent();
				H.Location = A->GetActorLocation();
				H.ImpactPoint = H.Location;
				OutHits.Add(H);
			}
		}

		return bAny;
	}

	// Sweep From->To
	return FDeliveryQueryHelpersV3::SweepMulti(
		World,
		Spec.Query,
		From,
		To,
		FQuat::Identity,
		CS,
		Params,
		OutHits,
		TEXT("Visibility"));
}


void UDeliveryDriver_MoverV3::DebugDraw(
	const FSpellExecContextV3& Ctx,
	const UDeliveryGroupRuntimeV3* Group,
	const FDeliveryPrimitiveSpecV3& Spec,
	const FVector& From,
	const FVector& To,
	const TArray<FHitResult>& Hits) const
{
	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return;
	}

	const FDeliveryDebugDrawConfigV3 DebugCfg = (Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);
	if (!DebugCfg.bEnable)
	{
		return;
	}

	const float Dur = DebugCfg.Duration;

	if (DebugCfg.bDrawPath)
	{
		DrawDebugLine(World, From, To, FColor::Yellow, false, Dur, 0, 2.0f);
	}

	if (DebugCfg.bDrawShape)
	{
		switch (Spec.Shape.Kind)
		{
		case EDeliveryShapeV3::Sphere:
			DrawDebugSphere(World, To, Spec.Shape.Radius, 12, FColor::Green, false, Dur, 0, 1.0f);
			break;
		case EDeliveryShapeV3::Capsule:
			DrawDebugCapsule(World, To, Spec.Shape.HalfHeight, Spec.Shape.Radius, FQuat::Identity, FColor::Green, false, Dur, 0, 1.0f);
			break;
		case EDeliveryShapeV3::Box:
			DrawDebugBox(World, To, Spec.Shape.Extents, FQuat::Identity, FColor::Green, false, Dur, 0, 1.0f);
			break;
		default:
			break;
		}
	}

	if (DebugCfg.bDrawHits)
	{
		for (const FHitResult& H : Hits)
		{
			DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, Dur);
		}
	}
}

void UDeliveryDriver_MoverV3::WriteHitsToTargetStore(
	const FSpellExecContextV3& Ctx,
	const UDeliveryGroupRuntimeV3* Group,
	const FDeliveryContextV3& PrimitiveCtx,
	const TArray<FHitResult>& Hits) const
{
	const FName OutSetName = ResolveOutTargetSetName(Group, PrimitiveCtx);
	if (OutSetName == NAME_None)
	{
		return;
	}

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

void UDeliveryDriver_MoverV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;
	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		UE_LOG(LogIMOPDeliveryMoverV3, Error, TEXT("Mover Start failed: World/Group null"));
		Stop(Ctx, Group, EDeliveryStopReasonV3::Failed);
		return;
	}

	PositionWS = PrimitiveCtx.FinalPoseWS.GetLocation();
	TraveledDistance = 0.f;
	PierceHits = 0;

	const FVector Fwd = PrimitiveCtx.FinalPoseWS.GetRotation().GetForwardVector().GetSafeNormal();
	const float Speed = FMath::Max(0.f, PrimitiveCtx.Spec.Mover.Speed);
	VelocityWS = Fwd * Speed;

	NextSimTimeSeconds = World->GetTimeSeconds();

	if (PrimitiveCtx.Spec.Events.bEmitStarted)
	{
		EmitPrimitiveStarted(Ctx, PrimitiveCtx.Spec.Events.ExtraTags);
	}

	UE_LOG(LogIMOPDeliveryMoverV3, Log, TEXT("Mover started: %s/%s"), *GroupHandle.DeliveryId.ToString(), *PrimitiveId.ToString());
}

bool UDeliveryDriver_MoverV3::StepSim(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float StepSeconds)
{
	if (!Group || StepSeconds <= 0.f)
	{
		return false;
	}

	const FDeliveryContextV3* Live = Group->PrimitiveCtxById.Find(PrimitiveId);
	const FDeliveryContextV3& PCtx = Live ? *Live : LocalCtx;

	const FDeliveryPrimitiveSpecV3& Spec = PCtx.Spec;
	const FDeliveryMoverConfigV3& M = Spec.Mover;

	// Max distance
	if (M.MaxDistance > 0.f && TraveledDistance >= M.MaxDistance)
	{
		Stop(Ctx, Group, EDeliveryStopReasonV3::DurationElapsed);
		return false;
	}

	// Update velocity based on motion kind
	switch (M.Motion)
	{
	case EMoverMotionKindV3::Homing:
	{
		FVector TargetLoc;
		if (GetHomingTargetLocation(Ctx, M, TargetLoc))
		{
			const FVector DesiredDir = (TargetLoc - PositionWS).GetSafeNormal();
			const FVector CurrentDir = VelocityWS.IsNearlyZero() ? DesiredDir : VelocityWS.GetSafeNormal();
			const float Strength = FMath::Clamp(M.HomingStrength, 0.f, 1000.f);
			const float Alpha = FMath::Clamp(Strength * StepSeconds, 0.f, 1.f);
			const FVector NewDir = FMath::Lerp(CurrentDir, DesiredDir, Alpha).GetSafeNormal();
			VelocityWS = NewDir * FMath::Max(0.f, M.Speed);
		}
		break;
	}
	case EMoverMotionKindV3::Ballistic:
	{
		const float G = -980.f * FMath::Max(0.f, M.GravityScale);
		VelocityWS.Z += G * StepSeconds;
		break;
	}
	case EMoverMotionKindV3::Straight:
	default:
		// keep constant velocity
		break;
	}

	const FVector From = PositionWS;
	const FVector To = PositionWS + VelocityWS * StepSeconds;
	
	TArray<FHitResult> Hits;
	const bool bAny = SweepMoveAndCollectHits(Ctx, Ctx.GetWorld(), Spec, From, To, Hits);

	if (bAny)
	{
		SortHitsDeterministic(Hits, From);
		WriteHitsToTargetStore(Ctx, Group, PCtx, Hits);
		DebugDraw(Ctx, Group, Spec, From, To, Hits);		

		UWorld* World = Ctx.GetWorld();
		const FDeliveryDebugDrawConfigV3 DebugCfg =
			(Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

		DebugDrawPrimitiveShape(World, Spec, PCtx.FinalPoseWS, DebugCfg);

		
		if (Spec.Events.bEmitHit)
		{
			EmitPrimitiveHit(Ctx, (float)Hits.Num(), nullptr, Spec.Events.ExtraTags);
		}


		if (M.bPierce)
		{
			PierceHits += Hits.Num();
			const int32 MaxPierce = M.MaxPierceHits;
			if (MaxPierce > 0 && PierceHits >= MaxPierce)
			{
				PositionWS = Hits[0].ImpactPoint;
				Stop(Ctx, Group, EDeliveryStopReasonV3::OnFirstHit);
				return false;
			}

			// continue through (no penetration resolution yet)
			PositionWS = To;
		}
		else
		{
			if (M.bStopOnHit)
			{
				PositionWS = Hits[0].ImpactPoint;
				Stop(Ctx, Group, EDeliveryStopReasonV3::OnFirstHit);
				return false;
			}
			PositionWS = To;
		}
	}
	else
	{
		DebugDraw(Ctx, Group, Spec, From, To, Hits);
		PositionWS = To;
	}

	TraveledDistance += FVector::Distance(From, PositionWS);

	// Write live pose back into group ctx (so other systems can read)
	if (Live)
	{
		FDeliveryContextV3& Mut = Group->PrimitiveCtxById.FindChecked(PrimitiveId);
		Mut.FinalPoseWS = FTransform(Mut.FinalPoseWS.GetRotation(), PositionWS, Mut.FinalPoseWS.GetScale3D());
	}

	return true;
}

void UDeliveryDriver_MoverV3::Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds)
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


	const FDeliveryContextV3* Live = Group->PrimitiveCtxById.Find(PrimitiveId);
	const FDeliveryContextV3& PCtx = Live ? *Live : LocalCtx;

	if (PCtx.Spec.Events.bEmitTick)
	{
		EmitPrimitiveTick(Ctx, DeltaSeconds, PCtx.Spec.Events.ExtraTags);
	}
	
	
	const float Now = World->GetTimeSeconds();
	const float Interval = FMath::Max(0.f, PCtx.Spec.Mover.TickInterval);

	if (Interval > 0.f)
	{
		if (Now < NextSimTimeSeconds)
		{
			return;
		}
		NextSimTimeSeconds = Now + Interval;
		StepSim(Ctx, Group, Interval);
	}
	else
	{
		StepSim(Ctx, Group, FMath::Max(0.f, DeltaSeconds));
	}
}

void UDeliveryDriver_MoverV3::Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}

	bActive = false;

	if (LocalCtx.Spec.Events.bEmitStopped)
	{
		EmitPrimitiveStopped(Ctx, Reason, LocalCtx.Spec.Events.ExtraTags);
	}


	UE_LOG(LogIMOPDeliveryMoverV3, Log, TEXT("Mover stopped: %s/%s inst=%d reason=%d traveled=%.1f pierce=%d"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		GroupHandle.InstanceIndex,
		(int32)Reason,
		TraveledDistance,
		PierceHits);
}
