#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"
#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/DeliveryEventContextV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "DrawDebugHelpers.h"
#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryDriverV3, Log, All);

static void SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From)
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

FTransform UDeliveryDriver_InstantQueryV3::ResolveAttachTransform(const FSpellExecContextV3& Ctx) const
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
				for (const FTargetRefV3& R : Set->Targets)
				{
					if (AActor* T = R.Actor.Get())
					{
						Sum += T->GetActorLocation();
						Count++;
					}
				}
				if (Count > 0)
				{
					FTransform Xf = FTransform::Identity;
					Xf.SetLocation(Sum / (float)Count);
					return Xf;
				}
			}
		}
		UE_LOG(LogIMOPDeliveryDriverV3, Warning, TEXT("InstantQuery ResolveAttachTransform: TargetSetCenter missing/empty (%s) -> fallback caster"),
			*A.TargetSetName.ToString());
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;
	}

	case EDeliveryAttachKindV3::Socket:
		UE_LOG(LogIMOPDeliveryDriverV3, Warning, TEXT("InstantQuery: Socket attach not implemented yet -> using caster transform (Socket=%s)"),
			*A.SocketName.ToString());
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;

	default:
		return Ctx.Caster ? Ctx.Caster->GetActorTransform() : FTransform::Identity;
	}
}

void UDeliveryDriver_InstantQueryV3::Start(const FSpellExecContextV3& Ctx, const FDeliveryContextV3& InDeliveryCtx)
{
	DeliveryCtx = InDeliveryCtx;
	bActive = true;

	UDeliverySubsystemV3* Subsys = Ctx.GetWorld() ? Ctx.GetWorld()->GetSubsystem<UDeliverySubsystemV3>() : nullptr;
	if (!Subsys)
	{
		UE_LOG(LogIMOPDeliveryDriverV3, Error, TEXT("InstantQuery Start failed: DeliverySubsystem missing"));
		bActive = false;
		return;
	}

	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

	// Emit Started
	{
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

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPDeliveryDriverV3, Error, TEXT("InstantQuery Start failed: World is null"));
		Stop(Ctx, EDeliveryStopReasonV3::Failed);
		return;
	}

	FVector From = FVector::ZeroVector;
	FVector Dir  = FVector::ForwardVector;

	// Prefer Rig pose if present (final: universal pose pipeline)
	FDeliveryRigEvalResultV3 RigOut;
	if (!DeliveryCtx.Spec.Rig.IsEmpty())
	{
		const FDeliveryRigPoseV3& P = FDeliveryRigPoseSelectorV3::SelectPose(RigOut, DeliveryCtx.EmitterIndex);
		From = P.Location;
		Dir  = (!P.Forward.IsNearlyZero() ? P.Forward : P.Rotation.Vector());
	}
	else
	{
		const FTransform OriginXf = ResolveAttachTransform(Ctx);
		From = OriginXf.GetLocation();
		Dir  = OriginXf.GetRotation().GetForwardVector();
	}

	const float Range = DeliveryCtx.Spec.InstantQuery.Range;


	const FVector To = From + Dir * Range;

	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawPath)
	{
		DrawDebugLine(World, From, To, FColor::Cyan, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.5f);
	}
	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawPath)
	{
		// Direction indicator (helps verify per-emitter forward)
		DrawDebugDirectionalArrow(
			World,
			From,
			From + (Dir.GetSafeNormal() * 120.f),
			25.f,
			FColor::Cyan,
			false,
			DeliveryCtx.Spec.DebugDraw.Duration,
			0,
			1.5f
		);
	}

	
	UE_LOG(LogIMOPDeliveryDriverV3, Log, TEXT("InstantQuery: From=%s To=%s Range=%.1f Profile=%s IgnoreCaster=%d MaxHits=%d Multi=%d"),
		*From.ToString(), *To.ToString(), Range, *DeliveryCtx.Spec.Query.CollisionProfile.ToString(),
		DeliveryCtx.Spec.Query.bIgnoreCaster ? 1 : 0,
		DeliveryCtx.Spec.InstantQuery.MaxHits,
		DeliveryCtx.Spec.InstantQuery.bMultiHit ? 1 : 0);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_InstantQuery), /*bTraceComplex*/ false);
	if (DeliveryCtx.Spec.Query.bIgnoreCaster && Ctx.Caster)
	{
		Params.AddIgnoredActor(Ctx.Caster);
	}

	TArray<FHitResult> Hits;
	const bool bHitAny = World->LineTraceMultiByProfile(Hits, From, To, DeliveryCtx.Spec.Query.CollisionProfile, Params);

	if (bHitAny)
	{
		SortHitsDeterministic(Hits, From);
	}

	const int32 MaxHits = FMath::Max(1, DeliveryCtx.Spec.InstantQuery.MaxHits);
	const bool bMulti = DeliveryCtx.Spec.InstantQuery.bMultiHit;

	TArray<FHitResult> Final;
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

	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawHits)
	{
		for (const FHitResult& H : Final)
		{
			DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, DeliveryCtx.Spec.DebugDraw.Duration);
			DrawDebugLine(World, H.ImpactPoint, H.ImpactPoint + H.ImpactNormal * 30.f, FColor::Yellow, false, DeliveryCtx.Spec.DebugDraw.Duration, 0, 1.0f);
		}
	}

	
	// Emit Hit (batch)
	{
		FDeliveryEventContextV3 Ev;
		Ev.Type = EDeliveryEventTypeV3::Hit;
		Ev.Handle = DeliveryCtx.Handle;
		Ev.PrimitiveId = "P0";
		Ev.DeliveryTags = DeliveryCtx.Spec.DeliveryTags;
		Ev.HitTags = DeliveryCtx.Spec.HitTags;
		Ev.HitTags.AppendTags(DeliveryCtx.Spec.EventHitTags.Hit);

		Ev.Caster = Ctx.Caster;

		Ev.Hits.Reserve(Final.Num());
		for (const FHitResult& H : Final)
		{
			FDeliveryHitV3 DH;
			DH.Target = H.GetActor();
			DH.Location = H.ImpactPoint;
			DH.Normal = H.ImpactNormal;
			Ev.Hits.Add(DH);
		}

		UE_LOG(LogIMOPDeliveryDriverV3, Verbose, TEXT("InstantQuery Hit: %d hits (raw=%d)"), Ev.Hits.Num(), Hits.Num());
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

			UE_LOG(LogIMOPDeliveryDriverV3, Log,
				TEXT("InstantQuery Writeback: TargetSet=%s Count=%d"),
				*DeliveryCtx.Spec.OutTargetSet.ToString(),
				OutSet.Targets.Num());
		}


		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Hit, Ev);
	}

	// InstantQuery stops immediately
	Stop(Ctx, EDeliveryStopReasonV3::OnFirstHit);
}

void UDeliveryDriver_InstantQueryV3::Stop(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}
	bActive = false;

	UDeliverySubsystemV3* Subsys = Ctx.GetWorld() ? Ctx.GetWorld()->GetSubsystem<UDeliverySubsystemV3>() : nullptr;
	if (Subsys)
	{
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

		Subsys->EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Stopped, Ev);
	}

	UE_LOG(LogIMOPDeliveryDriverV3, Log, TEXT("InstantQuery stopped: Id=%s Inst=%d Reason=%d"),
		*DeliveryCtx.Handle.DeliveryId.ToString(), DeliveryCtx.Handle.InstanceIndex, (int32)Reason);
}
