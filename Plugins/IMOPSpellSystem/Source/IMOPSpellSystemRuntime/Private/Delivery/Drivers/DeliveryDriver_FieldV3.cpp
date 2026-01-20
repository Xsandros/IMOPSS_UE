#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/DeliveryEventContextV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "Engine/OverlapResult.h"

#include "Engine/World.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryFieldV3, Log, All);

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

	UE_LOG(LogIMOPDeliveryFieldV3, Log,
		TEXT("Field Start: Id=%s Inst=%d Radius=%.1f TickInterval=%.2f"),
		*DeliveryCtx.Handle.DeliveryId.ToString(),
		DeliveryCtx.Handle.InstanceIndex,
		DeliveryCtx.Spec.Shape.Radius,
		DeliveryCtx.Spec.Field.TickInterval);

	// Emit Started
	FDeliveryEventContextV3 Ev;
	Ev.Type = EDeliveryEventTypeV3::Started;
	Ev.Handle = DeliveryCtx.Handle;
	Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
	Ev.HitTags = DeliveryCtx.Spec.HitTags;
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

	const FTransform Xf = ResolveAttachTransform(Ctx);
	const FVector Center = Xf.GetLocation();
	const float Radius = DeliveryCtx.Spec.Shape.Radius;

	TSet<TWeakObjectPtr<AActor>> NewSet;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Field), false);
	if (DeliveryCtx.Spec.Query.bIgnoreCaster && Ctx.Caster)
	{
		Params.AddIgnoredActor(Ctx.Caster);
	}

	TArray<FOverlapResult> Overlaps;
	const bool bAny = World->OverlapMultiByProfile(
		Overlaps,
		Center,
		FQuat::Identity,
		DeliveryCtx.Spec.Query.CollisionProfile,
		FCollisionShape::MakeSphere(Radius),
		Params
	);

	if (bAny)
	{
		for (const FOverlapResult& O : Overlaps)
		{
			if (AActor* A = O.GetActor())
			{
				NewSet.Add(A);
			}
		}
	}

	UDeliverySubsystemV3* Subsys = World->GetSubsystem<UDeliverySubsystemV3>();
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

	// === Enter ===
	if (DeliveryCtx.Spec.Field.bEmitEnterExit)
	{
		TArray<FDeliveryHitV3> EnterHits;
		for (const TWeakObjectPtr<AActor>& A : NewSet)
		{
			if (!CurrentSet.Contains(A))
			{
				FDeliveryHitV3 H;
				H.Target = A;
				H.Location = Center;
				EnterHits.Add(H);
			}
		}

		if (EnterHits.Num() > 0)
		{
			FDeliveryEventContextV3 Ev;
			Ev.Type = EDeliveryEventTypeV3::Enter;
			Ev.Handle = DeliveryCtx.Handle;
			Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
			Ev.HitTags = DeliveryCtx.Spec.HitTags;
			Ev.Caster = Ctx.Caster;
			Ev.Hits = MoveTemp(EnterHits);

			UE_LOG(LogIMOPDeliveryFieldV3, Verbose, TEXT("Field Enter: %d"), Ev.Hits.Num());
			Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Enter, Ev);
		}
	}

	// === Exit ===
	if (DeliveryCtx.Spec.Field.bEmitEnterExit)
	{
		TArray<FDeliveryHitV3> ExitHits;
		for (const TWeakObjectPtr<AActor>& A : CurrentSet)
		{
			if (!NewSet.Contains(A))
			{
				FDeliveryHitV3 H;
				H.Target = A;
				H.Location = Center;
				ExitHits.Add(H);
			}
		}

		if (ExitHits.Num() > 0)
		{
			FDeliveryEventContextV3 Ev;
			Ev.Type = EDeliveryEventTypeV3::Exit;
			Ev.Handle = DeliveryCtx.Handle;
			Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
			Ev.HitTags = DeliveryCtx.Spec.HitTags;
			Ev.Caster = Ctx.Caster;
			Ev.Hits = MoveTemp(ExitHits);

			UE_LOG(LogIMOPDeliveryFieldV3, Verbose, TEXT("Field Exit: %d"), Ev.Hits.Num());
			Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Exit, Ev);
		}
	}

	// === Stay ===
	if (DeliveryCtx.Spec.Field.bEmitStay && NewSet.Num() > 0)
	{
		TArray<FDeliveryHitV3> StayHits;
		for (const TWeakObjectPtr<AActor>& A : NewSet)
		{
			FDeliveryHitV3 H;
			H.Target = A;
			H.Location = Center;
			StayHits.Add(H);
		}

		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Stay;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.Caster = Ctx.Caster;
		Ev.Hits = MoveTemp(StayHits);

		UE_LOG(LogIMOPDeliveryFieldV3, VeryVerbose, TEXT("Field Stay: %d"), Ev.Hits.Num());
		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Stay, Ev);
	}

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
	Ev.StopReason = Reason;
	Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
	Ev.HitTags = DeliveryCtx.Spec.HitTags;
	Ev.Caster = Ctx.Caster;

	UE_LOG(LogIMOPDeliveryFieldV3, Log,
		TEXT("Field Stop: Id=%s Inst=%d Reason=%d"),
		*DeliveryCtx.Handle.DeliveryId.ToString(),
		DeliveryCtx.Handle.InstanceIndex,
		(int32)Reason);

	Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Stopped, Ev);
}
