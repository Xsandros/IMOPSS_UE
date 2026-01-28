#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionShape.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryFieldV3, Log, All);

FName UDeliveryDriver_FieldV3::ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
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

void UDeliveryDriver_FieldV3::BuildSortedActorsDeterministic(const TSet<TWeakObjectPtr<AActor>>& InSet, TArray<AActor*>& OutActors)
{
	OutActors.Reset();
	for (const TWeakObjectPtr<AActor>& W : InSet)
	{
		if (AActor* A = W.Get())
		{
			OutActors.Add(A);
		}
	}

	// Deterministic-ish order (stable within a run; good enough for MP server authority)
	OutActors.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetUniqueID() < B.GetUniqueID();
	});
}

void UDeliveryDriver_FieldV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;
	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	TimeSinceLastEval = 0.f;
	CurrentSet.Reset();

	// Primitive started event (consistent with other drivers)
	if (LocalCtx.Spec.Events.bEmitStarted)
	{
		EmitPrimitiveStarted(Ctx, LocalCtx.Spec.Events.ExtraTags);
	}


	// Evaluate immediately once so the field is “live” on start
	Evaluate(Ctx, Group);
}

void UDeliveryDriver_FieldV3::Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds)
{
	if (!bActive || !Group)
	{
		return;
	}

	// Pull latest primitive context (poses/motion/anchors) from group each tick
	if (const FDeliveryContextV3* Latest = Group->PrimitiveCtxById.Find(PrimitiveId))
	{
		LocalCtx = *Latest;
	}
	else
	{
		// Primitive no longer exists in group (stopped/removed)
		bActive = false;
		return;
	}

	
	// Emit primitive tick (for analytics/debug; cheap)
	if (LocalCtx.Spec.Events.bEmitTick)
	{
		EmitPrimitiveTick(Ctx, DeltaSeconds, LocalCtx.Spec.Events.ExtraTags);
	}


	TimeSinceLastEval += DeltaSeconds;

	const float Interval = FMath::Max(0.f, LocalCtx.Spec.Field.TickInterval);
	if (Interval > 0.f && TimeSinceLastEval < Interval)
	{
		return;
	}

	TimeSinceLastEval = 0.f;
	Evaluate(Ctx, Group);
}

void UDeliveryDriver_FieldV3::Evaluate(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group)
{
	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return;
	}

	if (const FDeliveryContextV3* Latest = Group->PrimitiveCtxById.Find(PrimitiveId))
	{
		LocalCtx = *Latest;
	}

	
	const FDeliveryPrimitiveSpecV3& Spec = LocalCtx.Spec;

	// Center: already includes AnchorRef -> FinalPoseWS
	const FVector Center = LocalCtx.FinalPoseWS.GetLocation();
	const FDeliveryDebugDrawConfigV3 DebugCfg =
		(Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

	DebugDrawPrimitiveShape(World, Spec, LocalCtx.FinalPoseWS, DebugCfg);
	
	// Collision profile
	const FName Profile = (Spec.Query.CollisionProfile.Name != NAME_None)
		? Spec.Query.CollisionProfile.Name
		: FName(TEXT("OverlapAllDynamic"));


	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Field), /*bTraceComplex*/ false);
	if (Spec.Query.bIgnoreCaster)
	{
		if (AActor* Caster = Ctx.GetCaster())
		{
			Params.AddIgnoredActor(Caster);
		}
	}

	FCollisionShape Shape;
	switch (Spec.Shape.Kind)
	{
		case EDeliveryShapeV3::Sphere:
			Shape = FCollisionShape::MakeSphere(FMath::Max(0.f, Spec.Shape.Radius));
			break;
		case EDeliveryShapeV3::Capsule:
			Shape = FCollisionShape::MakeCapsule(FMath::Max(0.f, Spec.Shape.Radius), FMath::Max(0.f, Spec.Shape.HalfHeight));
			break;
		case EDeliveryShapeV3::Box:
			Shape = FCollisionShape::MakeBox(Spec.Shape.Extents);
			break;
		default:
			// Fallback: sphere radius 100
			Shape = FCollisionShape::MakeSphere(100.f);
			break;
	}

	TArray<FOverlapResult> Overlaps;
	const bool bAny = World->OverlapMultiByProfile(
		Overlaps,
		Center,
		FQuat::Identity,
		Profile,
		Shape,
		Params
	);

	TSet<TWeakObjectPtr<AActor>> NewSet;
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

	// Debug draw
	if (DebugCfg.bEnable && DebugCfg.bDrawShape)
	{
		const float Dur = DebugCfg.Duration;

		if (Spec.Shape.Kind == EDeliveryShapeV3::Sphere)
		{
			DrawDebugSphere(World, Center, Shape.GetSphereRadius(), 16, FColor::Cyan, false, Dur, 0, 1.25f);
		}
		else if (Spec.Shape.Kind == EDeliveryShapeV3::Capsule)
		{
			DrawDebugCapsule(World, Center, Shape.GetCapsuleHalfHeight(), Shape.GetCapsuleRadius(), FQuat::Identity, FColor::Cyan, false, Dur, 0, 1.25f);
		}
		else if (Spec.Shape.Kind == EDeliveryShapeV3::Box)
		{
			DrawDebugBox(World, Center, Shape.GetBox(), FColor::Cyan, false, Dur, 0, 1.25f);
		}
	}

	// Writeback occupants -> TargetStore
	const FName OutSetName = ResolveOutTargetSetName(Group, LocalCtx);
	if (OutSetName != NAME_None)
	{
		if (USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get()))
		{
			TArray<AActor*> SortedActors;
			BuildSortedActorsDeterministic(NewSet, SortedActors);

			FTargetSetV3 Out;
			Out.Targets.Reserve(SortedActors.Num());

			for (AActor* A : SortedActors)
			{
				FTargetRefV3 R;
				R.Actor = A;
				Out.AddUnique(R);
			}

			Store->Set(OutSetName, Out);
		}
	}

	// ===== Events
	// Enter/Exit (optional)
	if (Spec.Field.bEmitEnterExit)
	{
		int32 EnterCount = 0;
		for (const TWeakObjectPtr<AActor>& W : NewSet)
		{
			if (!CurrentSet.Contains(W))
			{
				EnterCount++;
			}
		}

		int32 ExitCount = 0;
		for (const TWeakObjectPtr<AActor>& W : CurrentSet)
		{
			if (!NewSet.Contains(W))
			{
				ExitCount++;
			}
		}

		if (EnterCount > 0 && Spec.FieldEvents.bEmitEnter)
		{
			EmitPrimitiveEnter(Ctx, Spec.Events.ExtraTags);
		}
		if (ExitCount > 0 && Spec.FieldEvents.bEmitExit)
		{
			EmitPrimitiveExit(Ctx, Spec.Events.ExtraTags);
		}

	}

	// Stay + Hit semantics
	if (NewSet.Num() > 0)
	{
		if (Spec.FieldEvents.bEmitStay)
		{
			EmitPrimitiveStay(Ctx, Spec.Events.ExtraTags);
		}
		if (Spec.Events.bEmitHit)
		{
			EmitPrimitiveHit(Ctx, (float)NewSet.Num(), nullptr, Spec.Events.ExtraTags);
		}
	}

	UE_LOG(LogIMOPDeliveryFieldV3, Verbose, TEXT("Field Eval: %s/%s inside=%d any=%d center=%s"),
		*GroupHandle.DeliveryId.ToString(), *PrimitiveId.ToString(), NewSet.Num(), bAny ? 1 : 0, *Center.ToString());

	CurrentSet = MoveTemp(NewSet);
}

void UDeliveryDriver_FieldV3::Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* /*Group*/, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}

	bActive = false;
	CurrentSet.Reset();

	if (LocalCtx.Spec.Events.bEmitStopped)
	{
		EmitPrimitiveStopped(Ctx, Reason, LocalCtx.Spec.Events.ExtraTags);
	}


	UE_LOG(LogIMOPDeliveryFieldV3, Log, TEXT("Field stopped: %s/%s inst=%d reason=%d"),
		*GroupHandle.DeliveryId.ToString(), *PrimitiveId.ToString(), GroupHandle.InstanceIndex, (int32)Reason);
}
