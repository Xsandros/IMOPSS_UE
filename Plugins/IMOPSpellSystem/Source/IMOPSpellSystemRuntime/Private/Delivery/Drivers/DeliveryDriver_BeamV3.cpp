#include "Delivery/Drivers/DeliveryDriver_BeamV3.h"

#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/DeliveryEventContextV3.h"
#include "Core/SpellGameplayTagsV3.h"

#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryBeamV3, Log, All);

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

void UDeliveryDriver_BeamV3::BuildSortedActorsDeterministic(const TSet<TWeakObjectPtr<AActor>>& In, TArray<AActor*>& Out)
{
	Out.Reset();
	for (const TWeakObjectPtr<AActor>& W : In)
	{
		AActor* A = W.Get();
		if (!A) continue;
		Out.Add(A);
	}
	Out.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetUniqueID() < B.GetUniqueID();
	});
}

void UDeliveryDriver_BeamV3::BuildSortedActorsDeterministicFromHits(const TArray<FHitResult>& InHits, TArray<AActor*>& OutActors, const FVector& From)
{
	TArray<FHitResult> Hits = InHits;
	SortHitResultsDeterministic(Hits, From);

	OutActors.Reset();
	for (const FHitResult& H : Hits)
	{
		AActor* A = H.GetActor();
		if (!A) continue;

		bool bAlready = false;
		for (AActor* Existing : OutActors)
		{
			if (Existing == A) { bAlready = true; break; }
		}
		if (!bAlready)
		{
			OutActors.Add(A);
		}
	}
}

FTransform UDeliveryDriver_BeamV3::ResolveAttachTransform(const FSpellExecContextV3& Ctx) const
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
		UE_LOG(LogIMOPDeliveryBeamV3, Warning, TEXT("Beam ResolveAttachTransform: TargetSet invalid/empty (%s) -> fallback caster"),
			*A.TargetSetName.ToString());
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;
	}

	case EDeliveryAttachKindV3::ActorRef:
		UE_LOG(LogIMOPDeliveryBeamV3, Warning, TEXT("Beam Attach.ActorRef not implemented yet (ActorRefName=%s) -> fallback caster"),
			*A.ActorRefName.ToString());
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;

	case EDeliveryAttachKindV3::Socket:
		UE_LOG(LogIMOPDeliveryBeamV3, Warning, TEXT("Beam Attach.Socket not implemented yet (Socket=%s) -> fallback caster"),
			*A.SocketName.ToString());
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;

	default:
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;
	}
}

bool UDeliveryDriver_BeamV3::ResolveBeamOriginAndDir(const FSpellExecContextV3& Ctx, FVector& OutOrigin, FVector& OutDir) const
{
	// Prefer Rig root if present, else attach.
	FTransform Xf = FTransform::Identity;
	if (!DeliveryCtx.Spec.Rig.IsEmpty())
	{
		FDeliveryRigEvalResultV3 RigOut;
		FDeliveryRigEvaluatorV3::Evaluate(Ctx, DeliveryCtx, DeliveryCtx.Spec.Rig, RigOut);
		Xf = FTransform(RigOut.Root.Rotation, RigOut.Root.Location);
	}
	else
	{
		Xf = ResolveAttachTransform(Ctx);
	}

	OutOrigin = Xf.GetLocation();
	OutDir = Xf.GetRotation().GetForwardVector().GetSafeNormal();

	// Lock-on overrides direction (origin stays from rig/attach)
	if (DeliveryCtx.Spec.Beam.bLockOnTarget && Ctx.TargetStore && DeliveryCtx.Spec.Beam.LockTargetSet != NAME_None)
	{
		if (const FTargetSetV3* Set = Ctx.TargetStore->Find(DeliveryCtx.Spec.Beam.LockTargetSet))
		{
			AActor* BestActor = nullptr;
			int32 BestStable = 0;
			float BestD2 = 0.f;

			for (const FTargetRefV3& R : Set->Targets)
			{
				AActor* A = R.Actor.Get();
				if (!A) continue;

				const float D2 = FVector::DistSquared(OutOrigin, A->GetActorLocation());
				const int32 Stable = R.StableId();

				if (!BestActor || D2 < BestD2 || (D2 == BestD2 && Stable < BestStable))
				{
					BestActor = A;
					BestD2 = D2;
					BestStable = Stable;
				}
			}

			if (BestActor)
			{
				OutDir = (BestActor->GetActorLocation() - OutOrigin).GetSafeNormal();
			}
		}
	}

	return !OutDir.IsNearlyZero();
}

void UDeliveryDriver_BeamV3::EmitStarted(const FSpellExecContextV3& Ctx)
{
	if (UDeliverySubsystemV3* Subsys = Ctx.GetWorld() ? Ctx.GetWorld()->GetSubsystem<UDeliverySubsystemV3>() : nullptr)
	{
		const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Started;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = DeliveryCtx.PrimitiveId;
		Ev.StopReminder = EDeliveryStopReasonV3::Manual;
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Started);
		Ev.Caster = Ctx.Caster;

		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Started, Ev);
	}
}

void UDeliveryDriver_BeamV3::EmitStopped(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason)
{
	if (UDeliverySubsystemV3* Subsys = Ctx.GetWorld() ? Ctx.GetWorld()->GetSubsystem<UDeliverySubsystemV3>() : nullptr)
	{
		const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Stopped;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = DeliveryCtx.PrimitiveId;
		Ev.StopReminder = Reason;
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Stopped);
		Ev.Caster = Ctx.Caster;

		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Stopped, Ev);
	}
}

void UDeliveryDriver_BeamV3::WritebackTargets(const FSpellExecContextV3& Ctx, const TArray<AActor*>& Actors) const
{
	if (!Ctx.TargetStore) return;

	const FName Key = DeliveryCtx.Spec.OutTargetSet;
	if (Key == NAME_None) return;

	FTargetSetV3 Set;
	for (AActor* A : Actors)
	{
		if (!A) continue;
		FTargetRefV3 R;
		R.Actor = A;
		Set.AddUnique(R);
	}

	Ctx.TargetStore->Set(Key, Set);

	UE_LOG(LogIMOPDeliveryBeamV3, VeryVerbose, TEXT("Beam Writeback: TargetSet=%s Count=%d"), *Key.ToString(), Set.Targets.Num());
}

void UDeliveryDriver_BeamV3::EmitEnterExitStayTick(
	const FSpellExecContextV3& Ctx,
	const TArray<AActor*>& EnterActors,
	const TArray<AActor*>& ExitActors,
	const TArray<AActor*>& StayActors,
	bool bEmitTick)
{
	UDeliverySubsystemV3* Subsys = Ctx.GetWorld() ? Ctx.GetWorld()->GetSubsystem<UDeliverySubsystemV3>() : nullptr;
	if (!Subsys) return;

	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

	auto MakeHitsFromActors = [&](const TArray<AActor*>& InActors) -> TArray<FDeliveryHitV3>
	{
		TArray<FDeliveryHitV3> Out;
		Out.Reserve(InActors.Num());
		for (AActor* A : InActors)
		{
			if (!A) continue;
			FDeliveryHitV3 H;
			H.Target = A;
			H.Location = A->GetActorLocation();
			H.Normal = FVector::ZeroVector;
			Out.Add(H);
		}
		return Out;
	};

	if (EnterActors.Num() > 0)
	{
		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Enter;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = DeliveryCtx.PrimitiveId;
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Enter);
		Ev.Caster = Ctx.Caster;
		Ev.Hits = MakeHitsFromActors(EnterActors);

		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Enter, Ev);
	}

	if (ExitActors.Num() > 0)
	{
		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Exit;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = DeliveryCtx.PrimitiveId;
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Exit);
		Ev.Caster = Ctx.Caster;
		Ev.Hits = MakeHitsFromActors(ExitActors);

		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Exit, Ev);
	}

	if (DeliveryCtx.Spec.Field.bEmitStay && StayActors.Num() > 0)
	{
		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Stay;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = DeliveryCtx.PrimitiveId;
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Stay);
		Ev.Caster = Ctx.Caster;
		Ev.Hits = MakeHitsFromActors(StayActors);

		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Stay, Ev);

		// Writeback current inside set for downstream effects
		WritebackTargets(Ctx, StayActors);
	}

	if (bEmitTick)
	{
		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Tick;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = DeliveryCtx.PrimitiveId;
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Tick);
		Ev.Caster = Ctx.Caster;

		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Tick, Ev);
	}
}

void UDeliveryDriver_BeamV3::Start(const FSpellExecContextV3& Ctx, const FDeliveryContextV3& InDeliveryCtx)
{
	DeliveryCtx = InDeliveryCtx;
	bActive = true;

	TimeSinceStart = 0.f;
	TimeSinceLastEval = 0.f;
	InsideSet.Reset();

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPDeliveryBeamV3, Error, TEXT("Beam Start failed: World null"));
		Stop(Ctx, EDeliveryStopReasonV3::Failed);
		return;
	}

	UE_LOG(LogIMOPDeliveryBeamV3, Log,
		TEXT("Beam Start: Id=%s Inst=%d Prim=%s Range=%.1f Tick=%.3f Radius=%.1f Lock=%d LockSet=%s Profile=%s IgnoreCaster=%d MaxDur=%.2f"),
		*DeliveryCtx.Handle.DeliveryId.ToString(),
		DeliveryCtx.Handle.InstanceIndex,
		*DeliveryCtx.PrimitiveId.ToString(),
		DeliveryCtx.Spec.Beam.Range,
		DeliveryCtx.Spec.Beam.TickInterval,
		DeliveryCtx.Spec.Beam.Radius,
		DeliveryCtx.Spec.Beam.bLockOnTarget ? 1 : 0,
		*DeliveryCtx.Spec.Beam.LockTargetSet.ToString(),
		*DeliveryCtx.Spec.Query.CollisionProfile.ToString(),
		DeliveryCtx.Spec.Query.bIgnoreCaster ? 1 : 0,
		DeliveryCtx.Spec.StopPolicy.MaxDuration);

	EmitStarted(Ctx);
}

void UDeliveryDriver_BeamV3::Tick(const FSpellExecContextV3& Ctx, float DeltaSeconds)
{
	if (!bActive) return;

	const float DtFrame = FMath::Max(0.f, DeltaSeconds);
	TimeSinceStart += DtFrame;
	TimeSinceLastEval += DtFrame;

	// StopPolicy: MaxDuration
	if (DeliveryCtx.Spec.StopPolicy.MaxDuration > 0.f && TimeSinceStart >= DeliveryCtx.Spec.StopPolicy.MaxDuration)
	{
		UE_LOG(LogIMOPDeliveryBeamV3, Log, TEXT("Beam Stop: MaxDuration reached (%.3f)"), DeliveryCtx.Spec.StopPolicy.MaxDuration);
		Stop(Ctx, EDeliveryStopReasonV3::DurationElapsed);
		return;
	}

	// Interval eval
	const float Interval = FMath::Max(0.f, DeliveryCtx.Spec.Beam.TickInterval);
	if (Interval > 0.f && TimeSinceLastEval < Interval)
	{
		return;
	}
	TimeSinceLastEval = 0.f;

	FVector Origin = FVector::ZeroVector;
	FVector Dir = FVector::ForwardVector;
	if (!ResolveBeamOriginAndDir(Ctx, Origin, Dir))
	{
		UE_LOG(LogIMOPDeliveryBeamV3, Warning, TEXT("Beam Tick: could not resolve direction (Prim=%s)"), *DeliveryCtx.PrimitiveId.ToString());
		return;
	}

	const float Range = FMath::Max(0.f, DeliveryCtx.Spec.Beam.Range);

	// Debug draw ray
	if (DeliveryCtx.Spec.DebugDraw.bEnable)
	{
		if (DeliveryCtx.Spec.DebugDraw.bDrawPath)
		{
			DrawDebugLine(Ctx.GetWorld(), Origin, Origin + Dir * Range, FColor::Magenta, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 2.0f);
			DrawDebugDirectionalArrow(Ctx.GetWorld(), Origin, Origin + Dir * (Range * 0.2f), 40.f, FColor::Magenta, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 2.0f);
		}
	}

	EvaluateBeam(Ctx, Origin, Dir, Range);
}

void UDeliveryDriver_BeamV3::EvaluateBeam(const FSpellExecContextV3& Ctx, const FVector& Origin, const FVector& Dir, float Range)
{
	UWorld* World = Ctx.GetWorld();
	if (!World) return;

	const FVector To = Origin + Dir * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Beam), false);
	if (DeliveryCtx.Spec.Query.bIgnoreCaster && Ctx.Caster)
	{
		Params.AddIgnoredActor(Ctx.Caster);
	}

	TArray<FHitResult> Hits;

	const float Radius = FMath::Max(0.f, DeliveryCtx.Spec.Beam.Radius);
	if (Radius <= 0.f)
	{
		World->LineTraceMultiByProfile(Hits, Origin, To, DeliveryCtx.Spec.Query.CollisionProfile, Params);
	}
	else
	{
		const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
		World->SweepMultiByProfile(Hits, Origin, To, FQuat::Identity, DeliveryCtx.Spec.Query.CollisionProfile, Shape, Params);

		if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawShape)
		{
			DrawDebugSphere(World, Origin, Radius, 12, FColor::Magenta, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.0f);
			DrawDebugSphere(World, To, Radius, 12, FColor::Magenta, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.0f);
		}
	}

	if (Hits.Num() == 0)
	{
		// Still allow Tick heartbeat even with no hits
		EmitEnterExitStayTick(Ctx, TArray<AActor*>(), TArray<AActor*>(), TArray<AActor*>(), /*bEmitTick*/ true);
		return;
	}

	// determinism
	SortHitResultsDeterministic(Hits, Origin);

	// debug draw hitpoints
	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawHits)
	{
		for (const FHitResult& H : Hits)
		{
			DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, DeliveryCtx.Spec.DebugDraw.Duration);
			DrawDebugLine(World, H.ImpactPoint, H.ImpactPoint + H.ImpactNormal * 30.f, FColor::Yellow, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.0f);
		}
	}

	// Convert hits -> set of actors currently "inside beam"
	TSet<TWeakObjectPtr<AActor>> NewInside;
	for (const FHitResult& H : Hits)
	{
		AActor* A = H.GetActor();
		if (!A) continue;
		NewInside.Add(A);
	}

	// diff sets
	TArray<AActor*> EnterActors;
	TArray<AActor*> ExitActors;
	TArray<AActor*> StayActors;

	{
		// Enter = in New but not in Old
		TSet<TWeakObjectPtr<AActor>> EnterSet = NewInside;
		for (const TWeakObjectPtr<AActor>& Old : InsideSet)
		{
			EnterSet.Remove(Old);
		}
		BuildSortedActorsDeterministic(EnterSet, EnterActors);

		// Exit = in Old but not in New
		TSet<TWeakObjectPtr<AActor>> ExitSet = InsideSet;
		for (const TWeakObjectPtr<AActor>& N : NewInside)
		{
			ExitSet.Remove(N);
		}
		BuildSortedActorsDeterministic(ExitSet, ExitActors);

		// Stay = New (sorted)
		BuildSortedActorsDeterministic(NewInside, StayActors);
	}

	InsideSet = MoveTemp(NewInside);

	UE_LOG(LogIMOPDeliveryBeamV3, Verbose,
		TEXT("Beam Eval: Prim=%s inside=%d enter=%d exit=%d stay=%d origin=%s"),
		*DeliveryCtx.PrimitiveId.ToString(),
		InsideSet.Num(),
		EnterActors.Num(),
		ExitActors.Num(),
		StayActors.Num(),
		*Origin.ToString());

	// Emit batch events + tick heartbeat
	EmitEnterExitStayTick(Ctx, EnterActors, ExitActors, StayActors, /*bEmitTick*/ true);
}

void UDeliveryDriver_BeamV3::Stop(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason)
{
	if (!bActive) return;
	bActive = false;

	UE_LOG(LogIMOPDeliveryBeamV3, Log,
		TEXT("Beam Stopped: Id=%s Inst=%d Prim=%s Reason=%d"),
		*DeliveryCtx.Handle.DeliveryId.ToString(),
		DeliveryCtx.Handle.InstanceIndex,
		*DeliveryCtx.PrimitiveId.ToString(),
		(int32)Reason);

	EmitStopped(Ctx, Reason);
	InsideSet.Reset();
}
