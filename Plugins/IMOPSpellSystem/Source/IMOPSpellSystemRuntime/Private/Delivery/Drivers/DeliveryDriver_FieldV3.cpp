#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/DeliveryEventContextV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"

#include "Engine/World.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryFieldV3, Log, All);

static void BuildSortedActorsDeterministic(const TSet<TWeakObjectPtr<AActor>>& InSet, TArray<AActor*>& OutActors)
{
	OutActors.Reset();
	for (const TWeakObjectPtr<AActor>& W : InSet)
	{
		if (AActor* A = W.Get())
		{
			OutActors.Add(A);
		}
	}

	OutActors.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetUniqueID() < B.GetUniqueID();
	});
}


FTransform UDeliveryDriver_FieldV3::ResolveAttachTransform(const FSpellExecContextV3& Ctx) const
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
				FVector Sum = FVector::ZeroVector;
				int32 Count = 0;

				for (const FTargetRefV3& Ref : Set->Targets)
				{
					if (AActor* T = Ref.Actor.Get())
					{
						Sum += T->GetActorLocation();
						Count++;
					}
				}

				if (Count > 0)
				{
					FTransform Xf;
					Xf.SetLocation(Sum / (float)Count);
					return Xf;
				}
			}
		}
		UE_LOG(LogIMOPDeliveryFieldV3, Warning,
			TEXT("Field ResolveAttachTransform: TargetSetCenter invalid (%s), fallback caster"),
			*A.TargetSetName.ToString());

		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;
	}

	default:
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;
	}
}

void UDeliveryDriver_FieldV3::Start(const FSpellExecContextV3& Ctx, const FDeliveryContextV3& InDeliveryCtx)
{
	DeliveryCtx = InDeliveryCtx;
	bActive = true;
	TimeSinceLastEval = 0.f;
	CurrentSet.Reset();

	UDeliverySubsystemV3* Subsys = Ctx.GetWorld()->GetSubsystem<UDeliverySubsystemV3>();
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

	
	// Emit Started
	FDeliveryEventContextV3 Ev;
	Ev.Type = EDeliveryEventTypeV3::Started;
	Ev.Handle = DeliveryCtx.Handle;
	Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
	Ev.HitTags = DeliveryCtx.Spec.HitTags;
	Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Started);
	Ev.Caster = Ctx.Caster;

	Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Started, Ev);
}

void UDeliveryDriver_FieldV3::Tick(const FSpellExecContextV3& Ctx, float DeltaSeconds)
{
	if (!bActive)
	{
		return;
	}

	TimeSinceLastEval += DeltaSeconds;

	const float Interval = FMath::Max(0.01f, DeliveryCtx.Spec.Field.TickInterval);
	if (TimeSinceLastEval < Interval)
	{
		return;
	}

	TimeSinceLastEval = 0.f;
	Evaluate(Ctx);
}

void UDeliveryDriver_FieldV3::Evaluate(const FSpellExecContextV3& Ctx)
{
	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		return;
	}
	const float MaxDur = DeliveryCtx.Spec.StopPolicy.MaxDuration;
	if (MaxDur > 0.f)
	{
		const float Now = World->GetTimeSeconds(); 
		const float Elapsed = Now - DeliveryCtx.StartTime; 
		if (Elapsed >= MaxDur)
		{
			UE_LOG(LogIMOPDeliveryFieldV3, Log, TEXT("Field AutoStop: Id=%s Inst=%d MaxDur=%.3f Elapsed=%.3f"), 
				*DeliveryCtx.Handle.DeliveryId.ToString(), DeliveryCtx.Handle.InstanceIndex, MaxDur, Elapsed);
			Stop(Ctx, EDeliveryStopReasonV3::DurationElapsed); 
			return;
		}
	} 
	FTransform Xf = ResolveAttachTransform(Ctx);
	FVector Center = Xf.GetLocation();
	FDeliveryRigEvalResultV3 RigOut;
	if (!DeliveryCtx.Spec.Rig.IsEmpty())
	{
		FDeliveryRigEvaluatorV3::Evaluate(Ctx, DeliveryCtx, DeliveryCtx.Spec.Rig, RigOut);

		// Per-emitter center if spawned via multi-emitter
		const FDeliveryRigPoseV3& P = FDeliveryRigPoseSelectorV3::SelectPose(RigOut, DeliveryCtx.EmitterIndex);
		Center = P.Location;
	}
	else
	{
		const FTransform Xf2 = ResolveAttachTransform(Ctx);
		Center = Xf2.GetLocation();
	}

	float Radius = DeliveryCtx.Spec.Shape.Radius;

	if (Radius <= 0.f)
	{
		UE_LOG(LogIMOPDeliveryFieldV3, Warning,
			TEXT("Field Start: Radius <= 0 (%.2f) for Id=%s Inst=%d. Using fallback Radius=100."),
			Radius,
			*DeliveryCtx.Handle.DeliveryId.ToString(),
			DeliveryCtx.Handle.InstanceIndex);

		Radius = 100.f;
	}


	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawShape)
	{
		// Field currently uses sphere overlap; draw sphere for visibility
		DrawDebugSphere(World, Center, Radius, 16, FColor::Cyan, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.25f);
	}
	
	if (MaxDur > 0.f)
	{
		const float Now = Ctx.GetWorld() ? Ctx.GetWorld()->GetTimeSeconds() : DeliveryCtx.StartTime;
		const float Elapsed = Now - DeliveryCtx.StartTime;

		if (Elapsed >= MaxDur)
		{
			UE_LOG(LogIMOPDeliveryFieldV3, Log,
				TEXT("Field AutoStop: Id=%s Inst=%d MaxDur=%.3f Elapsed=%.3f"),
				*DeliveryCtx.Handle.DeliveryId.ToString(),
				DeliveryCtx.Handle.InstanceIndex,
				DeliveryCtx.Spec.StopPolicy.MaxDuration,
				Elapsed);

			Stop(Ctx, EDeliveryStopReasonV3::DurationElapsed);
			return;
		}
	}


	
	TSet<TWeakObjectPtr<AActor>> NewSet;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Field), false);
	if (DeliveryCtx.Spec.Query.bIgnoreCaster && Ctx.Caster)
	{
		Params.AddIgnoredActor(Ctx.Caster);
	}

	TArray<FOverlapResult> Overlaps;
	const bool bAny = World->OverlapMultiByProfile( Overlaps, Center, FQuat::Identity, DeliveryCtx.Spec.Query.CollisionProfile, FCollisionShape::MakeSphere(Radius), Params ); 
	UE_LOG(LogIMOPDeliveryFieldV3, Verbose, TEXT("Field Query: profile=%s radius=%.1f ignoreCaster=%d raw=%d any=%d center=%s"), *DeliveryCtx.Spec.Query.CollisionProfile.ToString(), Radius, DeliveryCtx.Spec.Query.bIgnoreCaster ? 1 : 0, Overlaps.Num(), bAny ? 1 : 0, *Center.ToString());
	if (bAny) 
	{ 
		for (const FOverlapResult& O : Overlaps)
		{
			if (AActor* A = O.GetActor()) { NewSet.Add(A); }
		}
	}


	// ================================
	// Target writeback (Inside/Current -> TargetStore), deterministic order
	// ================================
	if (Ctx.TargetStore && DeliveryCtx.Spec.OutTargetSet != NAME_None)
	{
		TArray<AActor*> SortedActors;
		BuildSortedActorsDeterministic(NewSet, SortedActors);

		FTargetSetV3 OutSet;
		OutSet.Targets.Reserve(SortedActors.Num());

		for (AActor* A : SortedActors)
		{
			FTargetRefV3 Ref;
			Ref.Actor = A;
			OutSet.AddUnique(Ref);
		}

		Ctx.TargetStore->Set(DeliveryCtx.Spec.OutTargetSet, OutSet);

		UE_LOG(LogIMOPDeliveryFieldV3, Log,
			TEXT("Field Writeback: TargetSet=%s Count=%d"),
			*DeliveryCtx.Spec.OutTargetSet.ToString(),
			OutSet.Targets.Num());
	}

	
	UDeliverySubsystemV3* Subsys = World->GetSubsystem<UDeliverySubsystemV3>();
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

	// Deterministic iteration order (TSet is not deterministic)
	TArray<AActor*> NewActors;
	BuildSortedActorsDeterministic(NewSet, NewActors);

	TArray<AActor*> OldActors;
	BuildSortedActorsDeterministic(CurrentSet, OldActors);
	int32 EnterCount = 0; 
	
	// === Enter === (deterministic)
	if (DeliveryCtx.Spec.Field.bEmitEnterExit)
	{
		TArray<FDeliveryHitV3> EnterHits;
		for (AActor* A : NewActors)
		{
			if (!A) { continue; }

			const TWeakObjectPtr<AActor> W(A);
			if (!CurrentSet.Contains(W))
			{
				FDeliveryHitV3 H;
				H.Target = W;
				H.Location = Center;
				EnterHits.Add(H);
			}
		}

		EnterCount = EnterHits.Num();
		if (EnterHits.Num() > 0)
		{
			FDeliveryEventContextV3 Ev;
			Ev.Type = EDeliveryEventTypeV3::Enter;
			Ev.Handle = DeliveryCtx.Handle;
			Ev.PrimitiveId = "P0";
			Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
			Ev.HitTags = DeliveryCtx.Spec.HitTags;
			Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Enter);
			Ev.Caster = Ctx.Caster;
			Ev.Hits = MoveTemp(EnterHits);

			UE_LOG(LogIMOPDeliveryFieldV3, Verbose, TEXT("Field Enter: %d"), Ev.Hits.Num());
			Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Enter, Ev);
		}
	}


	// === Exit === (deterministic)
	int32 ExitCount = 0;
	if (DeliveryCtx.Spec.Field.bEmitEnterExit)
	{
		TArray<FDeliveryHitV3> ExitHits;
		for (AActor* A : OldActors)
		{
			if (!A) { continue; }

			const TWeakObjectPtr<AActor> W(A);
			if (!NewSet.Contains(W))
			{
				FDeliveryHitV3 H;
				H.Target = W;
				H.Location = Center;
				ExitHits.Add(H);
			}
		}

		ExitCount = ExitHits.Num();
		if (ExitHits.Num() > 0)
		{
			FDeliveryEventContextV3 Ev;
			Ev.Type = EDeliveryEventTypeV3::Exit;
			Ev.Handle = DeliveryCtx.Handle;
			Ev.PrimitiveId = "P0";
			Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
			Ev.HitTags = DeliveryCtx.Spec.HitTags;
			Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Exit);
			Ev.Caster = Ctx.Caster;
			Ev.Hits = MoveTemp(ExitHits);

			UE_LOG(LogIMOPDeliveryFieldV3, Verbose, TEXT("Field Exit: %d"), Ev.Hits.Num());
			Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Exit, Ev);
		}
	}


	// === Stay === (deterministic)
	int32 StayCount = 0;
	if (DeliveryCtx.Spec.Field.bEmitStay && NewActors.Num() > 0)
	{
		TArray<FDeliveryHitV3> StayHits;
		StayHits.Reserve(NewActors.Num());

		for (AActor* A : NewActors)
		{
			if (!A) { continue; }

			FDeliveryHitV3 H;
			H.Target = A;
			H.Location = Center;
			StayHits.Add(H);
		}

		StayCount = StayHits.Num();

		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Stay;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = "P0";
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Stay);

		Ev.Caster = Ctx.Caster;
		Ev.Hits = MoveTemp(StayHits);

		UE_LOG(LogIMOPDeliveryFieldV3, VeryVerbose, TEXT("Field Stay: %d"), Ev.Hits.Num());
		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Stay, Ev);
	}

	UE_LOG(LogIMOPDeliveryFieldV3, Log, TEXT("Field Eval: prim=%s inside=%d enter=%d exit=%d stay=%d center=%s"),
		TEXT("P0"), NewActors.Num(), EnterCount, ExitCount, StayCount, *Center.ToString());


	CurrentSet = MoveTemp(NewSet);
}

void UDeliveryDriver_FieldV3::Stop(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}

	bActive = false;
	CurrentSet.Reset();

	UDeliverySubsystemV3* Subsys = Ctx.GetWorld()->GetSubsystem<UDeliverySubsystemV3>();
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

	FDeliveryEventContextV3 Ev;
	Ev.Type = EDeliveryEventTypeV3::Stopped;
	Ev.Handle = DeliveryCtx.Handle;
	Ev.PrimitiveId = "P0";
	Ev.StopReminder = Reason;
	Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
	Ev.HitTags = DeliveryCtx.Spec.HitTags;
	Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Stopped);
	Ev.Caster = Ctx.Caster;

	UE_LOG(LogIMOPDeliveryFieldV3, Log,
		TEXT("Field Stop: Id=%s Inst=%d Reason=%d"),
		*DeliveryCtx.Handle.DeliveryId.ToString(),
		DeliveryCtx.Handle.InstanceIndex,
		(int32)Reason);

	Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Stopped, Ev);
}
