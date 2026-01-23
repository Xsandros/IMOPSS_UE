#include "Delivery/Drivers/DeliveryDriver_MoverV3.h"

#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/DeliveryEventContextV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "DrawDebugHelpers.h"
#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryMoverV3, Log, All);

static void SortHitResultsDeterministic(TArray<FHitResult>& Hits, const FVector& From)
{
	Hits.Sort([&](const FHitResult& A, const FHitResult& B)
	{
		const float DA = FVector::DistSquared(From, A.ImpactPoint);
		const float DB = FVector::DistSquared(From, B.ImpactPoint);
		if (DA != DB) return DA < DB;

		const int32 IA = A.GetActor() ? A.GetActor()->GetUniqueID() : 0;
		const int32 IB = B.GetActor() ? B.GetActor()->GetUniqueID() : 0;
		return IA < IB;
	});
}

FTransform UDeliveryDriver_MoverV3::ResolveAttachTransform(const FSpellExecContextV3& Ctx) const
{
	const FDeliveryAttachV3& A = DeliveryCtx.Spec.Attach;

	switch (A.Kind)
	{
	case EDeliveryAttachKindV3::World:
		return A.WorldTransform;

	case EDeliveryAttachKindV3::Caster:
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;

	case EDeliveryAttachKindV3::TargetSet:
	{
		if (Ctx.TargetStore && A.TargetSetName != NAME_None)
		{
			if (const FTargetSetV3* Set = Ctx.TargetStore->Find(A.TargetSetName))
			{
				const FTargetRefV3* Best = nullptr;
				for (const FTargetRefV3& R : Set->Targets)
				{
					if (!R.IsValid()) continue;
					if (!Best || R.StableId() < Best->StableId())
					{
						Best = &R;
					}
				}
				if (Best && Best->Actor.IsValid())
				{
					return Best->Actor->GetActorTransform();
				}
			}
		}

		UE_LOG(LogIMOPDeliveryMoverV3, Warning,
			TEXT("Mover ResolveAttachTransform: TargetSet invalid/empty (%s) -> fallback caster"),
			*A.TargetSetName.ToString());

		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;
	}

	case EDeliveryAttachKindV3::ActorRef:
		UE_LOG(LogIMOPDeliveryMoverV3, Warning,
			TEXT("Mover Attach.ActorRef not implemented yet (ActorRefName=%s) -> fallback caster"),
			*A.ActorRefName.ToString());
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;

	case EDeliveryAttachKindV3::Socket:
		UE_LOG(LogIMOPDeliveryMoverV3, Warning,
			TEXT("Mover Attach.Socket not implemented yet (Socket=%s) -> fallback caster"),
			*A.SocketName.ToString());
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;

	default:
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;
	}
}

AActor* UDeliveryDriver_MoverV3::ChooseHomingTargetDeterministic(const FSpellExecContextV3& Ctx) const
{
	const FName Key = DeliveryCtx.Spec.Mover.HomingTargetSet;
	if (!Ctx.TargetStore || Key == NAME_None)
	{
		return nullptr;
	}

	const FTargetSetV3* Set = Ctx.TargetStore->Find(Key);
	if (!Set)
	{
		return nullptr;
	}

	AActor* BestActor = nullptr;
	float BestDist2 = 0.f;
	int32 BestStable = 0;

	for (const FTargetRefV3& R : Set->Targets)
	{
		AActor* A = R.Actor.Get();
		if (!A) continue;

		const float D2 = FVector::DistSquared(Position, A->GetActorLocation());
		const int32 Stable = R.StableId();

		if (!BestActor || D2 < BestDist2 || (D2 == BestDist2 && Stable < BestStable))
		{
			BestActor = A;
			BestDist2 = D2;
			BestStable = Stable;
		}
	}

	return BestActor;
}

void UDeliveryDriver_MoverV3::Start(const FSpellExecContextV3& Ctx, const FDeliveryContextV3& InDeliveryCtx)
{
	DeliveryCtx = InDeliveryCtx;
	bActive = true;

	DistanceTraveled = 0.f;
	TimeSinceStart = 0.f;
	TimeSinceLastEval = 0.f;
	PierceHitsSoFar = 0;
	UniqueHitTargets.Reset();

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPDeliveryMoverV3, Error, TEXT("Mover Start failed: World null"));
		Stop(Ctx, EDeliveryStopReasonV3::Failed);
		return;
	}

	FTransform StartXf = FTransform::Identity;

	FDeliveryRigEvalResultV3 RigOut;
	bool bHasRig = false;

	if (!DeliveryCtx.Spec.Rig.IsEmpty())
	{
		bHasRig = true;
		FDeliveryRigEvaluatorV3::Evaluate(Ctx, DeliveryCtx, DeliveryCtx.Spec.Rig, RigOut);

		const FDeliveryRigPoseV3& P = FDeliveryRigPoseSelectorV3::SelectPose(RigOut, DeliveryCtx.EmitterIndex);
		StartXf = FTransform(P.Rotation, P.Location);
	}
	else
	{
		StartXf = ResolveAttachTransform(Ctx);
	}

	Position = StartXf.GetLocation();

	FVector Forward = StartXf.GetRotation().GetForwardVector();
	if (bHasRig)
	{
		const FDeliveryRigPoseV3& P = FDeliveryRigPoseSelectorV3::SelectPose(RigOut, DeliveryCtx.EmitterIndex);
		if (!P.Forward.IsNearlyZero())
		{
			Forward = P.Forward.GetSafeNormal();
		}
	}
	Velocity = Forward * FMath::Max(0.f, DeliveryCtx.Spec.Mover.Speed);




	UE_LOG(LogIMOPDeliveryMoverV3, Log,
		TEXT("Mover Start: Id=%s Inst=%d Motion=%d Speed=%.1f MaxDist=%.1f MaxDur=%.1f Grav=%.2f HomingStr=%.2f HomingSet=%s StopOnHit=%d Pierce=%d MaxPierce=%d TickInterval=%.3f Shape=%d Profile=%s IgnoreCaster=%d"),
		*DeliveryCtx.Handle.DeliveryId.ToString(),
		DeliveryCtx.Handle.InstanceIndex,
		(int32)DeliveryCtx.Spec.Mover.Motion,
		DeliveryCtx.Spec.Mover.Speed,
		DeliveryCtx.Spec.Mover.MaxDistance,
		DeliveryCtx.Spec.StopPolicy.MaxDuration,
		DeliveryCtx.Spec.Mover.GravityScale,
		DeliveryCtx.Spec.Mover.HomingStrength,
		*DeliveryCtx.Spec.Mover.HomingTargetSet.ToString(),
		DeliveryCtx.Spec.Mover.bStopOnHit ? 1 : 0,
		DeliveryCtx.Spec.Mover.bPierce ? 1 : 0,
		DeliveryCtx.Spec.Mover.MaxPierceHits,
		DeliveryCtx.Spec.Mover.TickInterval,
		(int32)DeliveryCtx.Spec.Shape.Kind,
		*DeliveryCtx.Spec.Query.CollisionProfile.ToString(),
		DeliveryCtx.Spec.Query.bIgnoreCaster ? 1 : 0
	);

	// Emit Started
	if (UDeliverySubsystemV3* Subsys = World->GetSubsystem<UDeliverySubsystemV3>())
	{
		const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Started;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = "P0";
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Started);
		Ev.Caster = Ctx.Caster;

		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Started, Ev);
	}
}

void UDeliveryDriver_MoverV3::Tick(const FSpellExecContextV3& Ctx, float DeltaSeconds)
{
	if (!bActive)
	{
		return;
	}

	const float DtFrame = FMath::Max(0.f, DeltaSeconds);
	TimeSinceStart += DtFrame;
	TimeSinceLastEval += DtFrame;

	// StopPolicy: MaxDuration
	if (DeliveryCtx.Spec.StopPolicy.MaxDuration > 0.f && TimeSinceStart >= DeliveryCtx.Spec.StopPolicy.MaxDuration)
	{
		UE_LOG(LogIMOPDeliveryMoverV3, Log, TEXT("Mover Stop: MaxDuration reached (%.3f)"), DeliveryCtx.Spec.StopPolicy.MaxDuration);
		Stop(Ctx, EDeliveryStopReasonV3::DurationElapsed);
		return;
	}

	// TickInterval (0 => per frame)
	const float Interval = DeliveryCtx.Spec.Mover.TickInterval;
	if (Interval > 0.f && TimeSinceLastEval < Interval)
	{
		return;
	}
	TimeSinceLastEval = 0.f;

	const float DtSim = (Interval > 0.f) ? Interval : DtFrame;

	const FVector PrevPos = Position;

	// General, time-aware pose follow via Rig/Attach:
	// - OnStart  => use IntegrateMotion (classic projectile)
	// - EveryTick/Interval => update Position from Rig/Attach (kinematic), then sweep Prev->New
	const bool bHasRig = !DeliveryCtx.Spec.Rig.IsEmpty();
	const EDeliveryPoseUpdatePolicyV3 Policy = DeliveryCtx.Spec.PoseUpdatePolicy;

	bool bKinematicPose = (Policy != EDeliveryPoseUpdatePolicyV3::OnStart);

	// Interval gating for pose updates (optional)
	if (bKinematicPose && Policy == EDeliveryPoseUpdatePolicyV3::Interval && DeliveryCtx.Spec.PoseUpdateInterval > 0.f)
	{
		DeliveryCtx.PoseAccum += DtFrame;
		if (DeliveryCtx.PoseAccum < DeliveryCtx.Spec.PoseUpdateInterval)
		{
			// No pose update this frame; treat as no movement
		}
		else
		{
			DeliveryCtx.PoseAccum = 0.f;
		}
	}

	// Apply pose update if kinematic and either EveryTick, or Interval fired, or no interval configured
	const bool bDoPoseUpdate =
		bKinematicPose &&
		(Policy == EDeliveryPoseUpdatePolicyV3::EveryTick ||
		 DeliveryCtx.Spec.PoseUpdateInterval <= 0.f ||
		 DeliveryCtx.PoseAccum == 0.f);

	if (bDoPoseUpdate)
	{
		if (bHasRig)
		{
			FDeliveryRigEvalResultV3 RigOut;
			FDeliveryRigEvaluatorV3::Evaluate(Ctx, DeliveryCtx, DeliveryCtx.Spec.Rig, RigOut);
			const FDeliveryRigPoseV3& P = FDeliveryRigPoseSelectorV3::SelectPose(RigOut, DeliveryCtx.EmitterIndex);
			Position = P.Location;
		}
		else
		{
			const FTransform Xf = ResolveAttachTransform(Ctx);
			Position = Xf.GetLocation();
		}
	}
	else if (!bKinematicPose)
	{
		IntegrateMotion(Ctx, DtSim);
	}

	const FVector NewPos = Position;


	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawPath)
	{
		DrawDebugLine(Ctx.GetWorld(), PrevPos, NewPos, FColor::Cyan, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 2.0f);
	}

	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawPath)
	{
		const FVector Step = (NewPos - PrevPos);
		const FVector Dir = Step.IsNearlyZero() ? (Velocity.IsNearlyZero() ? FVector::ForwardVector : Velocity.GetSafeNormal()) : Step.GetSafeNormal();

		DrawDebugDirectionalArrow(
			Ctx.GetWorld(),
			NewPos,
			NewPos + (Dir * 120.f),
			25.f,
			FColor::Cyan,
			false,
			DeliveryCtx.Spec.DebugDraw.Duration,
			0,
			2.0f
		);
	}

	
	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawShape)
	{
		switch (DeliveryCtx.Spec.Shape.Kind)
		{
		case EDeliveryShapeV3::Sphere:
			DrawDebugSphere(Ctx.GetWorld(), NewPos, DeliveryCtx.Spec.Shape.Radius, 12, FColor::Cyan, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.0f);
			break;
		case EDeliveryShapeV3::Capsule:
			DrawDebugCapsule(Ctx.GetWorld(), NewPos, DeliveryCtx.Spec.Shape.HalfHeight, DeliveryCtx.Spec.Shape.Radius, FQuat::Identity, FColor::Cyan, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.0f);
			break;
		case EDeliveryShapeV3::Box:
			DrawDebugBox(Ctx.GetWorld(), NewPos, DeliveryCtx.Spec.Shape.Extents, FColor::Cyan, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.0f);
			break;
		default:
			break;
		}
	}

	
	const float StepDist = FVector::Distance(PrevPos, NewPos);
	DistanceTraveled += StepDist;

	UE_LOG(LogIMOPDeliveryMoverV3, VeryVerbose, TEXT("Mover Step: Prev=%s New=%s Step=%.2f Traveled=%.2f"),
		*PrevPos.ToString(), *NewPos.ToString(), StepDist, DistanceTraveled);

	// MaxDistance
	if (DeliveryCtx.Spec.Mover.MaxDistance > 0.f && DistanceTraveled >= DeliveryCtx.Spec.Mover.MaxDistance)
	{
		UE_LOG(LogIMOPDeliveryMoverV3, Log, TEXT("Mover Stop: MaxDistance reached (%.1f)"), DeliveryCtx.Spec.Mover.MaxDistance);
		Stop(Ctx, EDeliveryStopReasonV3::DurationElapsed);
		return;
	}

	EvaluateSweep(Ctx, PrevPos, NewPos);
}

void UDeliveryDriver_MoverV3::IntegrateMotion(const FSpellExecContextV3& Ctx, float Dt)
{
	const float Speed = FMath::Max(0.f, DeliveryCtx.Spec.Mover.Speed);

	switch (DeliveryCtx.Spec.Mover.Motion)
	{
	case EMoverMotionKindV3::Straight:
	{
		const FVector Dir = Velocity.IsNearlyZero() ? FVector::ForwardVector : Velocity.GetSafeNormal();
		Velocity = Dir * Speed;
		Position += Velocity * Dt;
		break;
	}
	case EMoverMotionKindV3::Ballistic:
	{
		const FVector Dir = Velocity.IsNearlyZero() ? FVector::ForwardVector : Velocity.GetSafeNormal();
		Velocity = Dir * Speed;

		const float GravScale = DeliveryCtx.Spec.Mover.GravityScale;
		const float Gz = Ctx.GetWorld() ? Ctx.GetWorld()->GetGravityZ() : -980.f;
		Velocity.Z += (Gz * GravScale) * Dt;

		Position += Velocity * Dt;
		break;
	}
	case EMoverMotionKindV3::Homing:
	{
		AActor* Target = ChooseHomingTargetDeterministic(Ctx);

		const FVector DesiredDir = Target
			? (Target->GetActorLocation() - Position).GetSafeNormal()
			: (Velocity.IsNearlyZero() ? FVector::ForwardVector : Velocity.GetSafeNormal());

		const FVector CurrentDir = Velocity.IsNearlyZero() ? DesiredDir : Velocity.GetSafeNormal();

		const float Strength = FMath::Max(0.f, DeliveryCtx.Spec.Mover.HomingStrength);
		const float Alpha = FMath::Clamp(Strength * Dt, 0.f, 1.f);
		const FVector NewDir = (CurrentDir * (1.f - Alpha) + DesiredDir * Alpha).GetSafeNormal();

		Velocity = NewDir * Speed;
		Position += Velocity * Dt;
		break;
	}
	default:
		Position += Velocity * Dt;
		break;
	}
}

void UDeliveryDriver_MoverV3::EvaluateSweep(const FSpellExecContextV3& Ctx, const FVector& From, const FVector& To)
{
	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		return;
	}

	FCollisionShape Shape;
	switch (DeliveryCtx.Spec.Shape.Kind)
	{
	case EDeliveryShapeV3::Sphere:
		Shape = FCollisionShape::MakeSphere(FMath::Max(0.f, DeliveryCtx.Spec.Shape.Radius));
		break;
	case EDeliveryShapeV3::Capsule:
		Shape = FCollisionShape::MakeCapsule(FMath::Max(0.f, DeliveryCtx.Spec.Shape.Radius), FMath::Max(0.f, DeliveryCtx.Spec.Shape.HalfHeight));
		break;
	case EDeliveryShapeV3::Box:
		Shape = FCollisionShape::MakeBox(DeliveryCtx.Spec.Shape.Extents);
		break;
	default:
		UE_LOG(LogIMOPDeliveryMoverV3, Warning, TEXT("Mover Sweep: unsupported shape kind=%d; using small sphere fallback"), (int32)DeliveryCtx.Spec.Shape.Kind);
		Shape = FCollisionShape::MakeSphere(FMath::Max(1.f, DeliveryCtx.Spec.Shape.Radius));
		break;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Mover), false);
	if (DeliveryCtx.Spec.Query.bIgnoreCaster && Ctx.Caster)
	{
		Params.AddIgnoredActor(Ctx.Caster);
	}

	TArray<FHitResult> Hits;
	const bool bAny = World->SweepMultiByProfile(
		Hits,
		From,
		To,
		FQuat::Identity,
		DeliveryCtx.Spec.Query.CollisionProfile,
		Shape,
		Params
	);

	if (!bAny || Hits.Num() == 0)
	{
		return;
	}

	SortHitResultsDeterministic(Hits, From);

	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawHits)
	{
		for (const FHitResult& H : Hits)
		{
			DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, DeliveryCtx.Spec.DebugDraw.Duration);
			DrawDebugLine(World, H.ImpactPoint, H.ImpactPoint + H.ImpactNormal * 30.f, FColor::Yellow, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.0f);
		}
	}

	
	TArray<FDeliveryHitV3> OutHits;
	OutHits.Reserve(Hits.Num());

	for (const FHitResult& H : Hits)
	{
		AActor* A = H.GetActor();
		if (!A) continue;

		// Always unique to avoid spam; we can later expose a config toggle if you want.
		if (UniqueHitTargets.Contains(A))
		{
			continue;
		}
		UniqueHitTargets.Add(A);

		FDeliveryHitV3 DH;
		DH.Target = A;
		DH.Location = H.ImpactPoint;
		DH.Normal = H.ImpactNormal;
		OutHits.Add(DH);

		if (DeliveryCtx.Spec.Mover.bPierce)
		{
			PierceHitsSoFar++;
			if (DeliveryCtx.Spec.Mover.MaxPierceHits > 0 && PierceHitsSoFar >= DeliveryCtx.Spec.Mover.MaxPierceHits)
			{
				break;
			}
		}
		else
		{
			// not piercing: only first target per eval
			break;
		}
	}

	if (OutHits.Num() == 0)
	{
		return;
	}

	UE_LOG(LogIMOPDeliveryMoverV3, Verbose,
		TEXT("Mover Hit: raw=%d filtered=%d StopOnHit=%d Pierce=%d PierceSoFar=%d Unique=%d"),
		Hits.Num(),
		OutHits.Num(),
		DeliveryCtx.Spec.Mover.bStopOnHit ? 1 : 0,
		DeliveryCtx.Spec.Mover.bPierce ? 1 : 0,
		PierceHitsSoFar,
		UniqueHitTargets.Num()
	);

	// Emit Hit batch
	if (UDeliverySubsystemV3* Subsys = World->GetSubsystem<UDeliverySubsystemV3>())
	{
		const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Hit;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = "P0";
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Hit);
		Ev.Caster = Ctx.Caster;
		Ev.Hits = MoveTemp(OutHits);

		// ================================
		// Target writeback (Hit -> TargetStore)
		// ================================
		if (Ctx.TargetStore && DeliveryCtx.Spec.OutTargetSet != NAME_None)
		{
			FTargetSetV3 OutSet;
			OutSet.Targets.Reserve(Ev.Hits.Num());

			for (const FDeliveryHitV3& DH : Ev.Hits)
			{
				if (!DH.Target.IsValid())
				{
					continue;
				}

				FTargetRefV3 Ref;
				Ref.Actor = DH.Target;
				OutSet.AddUnique(Ref);
			}

			Ctx.TargetStore->Set(DeliveryCtx.Spec.OutTargetSet, OutSet);

			UE_LOG(LogIMOPDeliveryMoverV3, Log,
				TEXT("Mover Writeback: TargetSet=%s Count=%d"),
				*DeliveryCtx.Spec.OutTargetSet.ToString(),
				OutSet.Targets.Num());
		}		
		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Hit, Ev);
	}

	// Stop on hit behavior
	if (DeliveryCtx.Spec.Mover.bStopOnHit && !DeliveryCtx.Spec.Mover.bPierce)
	{
		Stop(Ctx, EDeliveryStopReasonV3::OnFirstHit);
		return;
	}

	// Stop if pierce cap reached
	if (DeliveryCtx.Spec.Mover.bPierce && DeliveryCtx.Spec.Mover.MaxPierceHits > 0 && PierceHitsSoFar >= DeliveryCtx.Spec.Mover.MaxPierceHits)
	{
		Stop(Ctx, EDeliveryStopReasonV3::OnFirstHit);
		return;
	}
}

void UDeliveryDriver_MoverV3::Stop(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}
	bActive = false;

	UWorld* World = Ctx.GetWorld();
	if (World)
	{
		if (UDeliverySubsystemV3* Subsys = World->GetSubsystem<UDeliverySubsystemV3>())
		{
			const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

			FDeliveryEventContextV3 Ev;
			Ev.Type = EDeliveryEventTypeV3::Stopped;
			Ev.Handle = DeliveryCtx.Handle;
			Ev.PrimitiveId = "P0";
			Ev.StopReminder = Reason; // matches your struct
			Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
			Ev.HitTags = DeliveryCtx.Spec.HitTags;
			Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Stopped);
			Ev.Caster = Ctx.Caster;

			Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Stopped, Ev);
		}
	}

	UE_LOG(LogIMOPDeliveryMoverV3, Log,
		TEXT("Mover Stopped: Id=%s Inst=%d Reason=%d Traveled=%.2f UniqueHits=%d PierceHits=%d"),
		*DeliveryCtx.Handle.DeliveryId.ToString(),
		DeliveryCtx.Handle.InstanceIndex,
		(int32)Reason,
		DistanceTraveled,
		UniqueHitTargets.Num(),
		PierceHitsSoFar
	);
}
