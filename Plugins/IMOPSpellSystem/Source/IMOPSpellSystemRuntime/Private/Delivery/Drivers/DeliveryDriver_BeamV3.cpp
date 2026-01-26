#include "Delivery/Drivers/DeliveryDriver_BeamV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "Actions/SpellActionExecutorV3.h"
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

void UDeliveryDriver_BeamV3::SortActorsDeterministic(TArray<AActor*>& Actors)
{
	Actors.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetUniqueID() < B.GetUniqueID();
	});
}

AActor* UDeliveryDriver_BeamV3::PickLockTargetDeterministic(const FSpellExecContextV3& Ctx, FName TargetSetName)
{
	if (TargetSetName == NAME_None)
	{
		return nullptr;
	}

	USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get());
	if (!Store)
	{
		return nullptr;
	}

	const FTargetSetV3* Set = Store->Find(TargetSetName);
	if (!Set || Set->Targets.Num() == 0)
	{
		return nullptr;
	}

	// Deterministic pick: smallest StableId
	const FTargetRefV3* Best = nullptr;
	for (const FTargetRefV3& R : Set->Targets)
	{
		if (!R.IsValid())
		{
			continue;
		}
		if (!Best || R.StableId() < Best->StableId())
		{
			Best = &R;
		}
	}

	return (Best && Best->Actor.IsValid()) ? Best->Actor.Get() : nullptr;
}

void UDeliveryDriver_BeamV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;
	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	TimeSinceLastEval = 0.f;
	InsideSet.Reset();

	EmitPrimitiveStarted(Ctx);

	// Evaluate immediately so beam is “live” on start
	EvaluateBeam(Ctx, Group);
}

void UDeliveryDriver_BeamV3::Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds)
{
	if (!bActive || !Group)
	{
		return;
	}

	EmitPrimitiveTick(Ctx, DeltaSeconds);

	TimeSinceLastEval += DeltaSeconds;

	const float Interval = FMath::Max(0.f, LocalCtx.Spec.Beam.TickInterval);
	if (Interval > 0.f && TimeSinceLastEval < Interval)
	{
		return;
	}

	TimeSinceLastEval = 0.f;
	EvaluateBeam(Ctx, Group);
}

void UDeliveryDriver_BeamV3::EvaluateBeam(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group)
{
	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return;
	}

	const FDeliveryPrimitiveSpecV3& Spec = LocalCtx.Spec;
	const FDeliveryBeamConfigV3& B = Spec.Beam;

	const FVector Origin = LocalCtx.FinalPoseWS.GetLocation();

	// Direction from pose; optionally lock on target
	FVector Dir = LocalCtx.FinalPoseWS.GetRotation().GetForwardVector().GetSafeNormal();
	if (B.bLockOnTarget && B.LockTargetSet != NAME_None)
	{
		if (AActor* Target = PickLockTargetDeterministic(Ctx, B.LockTargetSet))
		{
			const FVector ToT = (Target->GetActorLocation() - Origin);
			if (!ToT.IsNearlyZero())
			{
				Dir = ToT.GetSafeNormal();
			}
		}
	}

	const float Range = FMath::Max(0.f, B.Range);
	const FVector End = Origin + Dir * Range;

	// Collision profile
	const FName Profile = (Spec.Query.CollisionProfile != NAME_None) ? Spec.Query.CollisionProfile : FName("Visibility");

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Beam), /*bTraceComplex*/ false);
	if (Spec.Query.bIgnoreCaster)
	{
		if (AActor* Caster = Ctx.GetCaster())
		{
			Params.AddIgnoredActor(Caster);
		}
	}

	TArray<FHitResult> Hits;
	bool bAny = false;

	const float Radius = FMath::Max(0.f, B.Radius);
	if (Radius <= 0.f)
	{
		bAny = World->LineTraceMultiByProfile(Hits, Origin, End, Profile, Params);
	}
	else
	{
		const FCollisionShape CS = FCollisionShape::MakeSphere(Radius);
		bAny = World->SweepMultiByProfile(Hits, Origin, End, FQuat::Identity, Profile, CS, Params);
	}

	// Convert to unique actor set (ignore nulls)
	TSet<TWeakObjectPtr<AActor>> NewInside;
	for (const FHitResult& H : Hits)
	{
		if (AActor* A = H.GetActor())
		{
			NewInside.Add(A);
		}
	}

	// Debug draw
	const FDeliveryDebugDrawConfigV3 DebugCfg = (Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);
	if (DebugCfg.bEnable)
	{
		const float Dur = DebugCfg.Duration;

		if (DebugCfg.bDrawPath)
		{
			DrawDebugLine(World, Origin, End, FColor::Cyan, false, Dur, 0, 2.0f);
			DrawDebugDirectionalArrow(World, Origin, Origin + Dir * 120.f, 25.f, FColor::Cyan, false, Dur, 0, 2.0f);
		}

		if (DebugCfg.bDrawHits && NewInside.Num() > 0)
		{
			for (const TWeakObjectPtr<AActor>& W : NewInside)
			{
				if (AActor* A = W.Get())
				{
					DrawDebugPoint(World, A->GetActorLocation(), 10.f, FColor::Red, false, Dur);
				}
			}
		}
	}

	// Writeback targets
	const FName OutSetName = ResolveOutTargetSetName(Group, LocalCtx);
	if (OutSetName != NAME_None)
	{
		if (USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get()))
		{
			TArray<AActor*> Actors;
			Actors.Reserve(NewInside.Num());
			for (const TWeakObjectPtr<AActor>& W : NewInside)
			{
				if (AActor* A = W.Get())
				{
					Actors.Add(A);
				}
			}

			SortActorsDeterministic(Actors);

			FTargetSetV3 Out;
			Out.Targets.Reserve(Actors.Num());

			for (AActor* A : Actors)
			{
				FTargetRefV3 R;
				R.Actor = A;
				Out.AddUnique(R);
			}

			Store->Set(OutSetName, Out);
		}
	}

	// ===== Events
	// Enter/Exit computed by set difference
	int32 EnterCount = 0;
	for (const TWeakObjectPtr<AActor>& W : NewInside)
	{
		if (!InsideSet.Contains(W))
		{
			EnterCount++;
		}
	}

	int32 ExitCount = 0;
	for (const TWeakObjectPtr<AActor>& W : InsideSet)
	{
		if (!NewInside.Contains(W))
		{
			ExitCount++;
		}
	}

	if (EnterCount > 0)
	{
		EmitPrimitiveEnter(Ctx);
	}
	if (ExitCount > 0)
	{
		EmitPrimitiveExit(Ctx);
	}

	if (NewInside.Num() > 0)
	{
		EmitPrimitiveStay(Ctx);
		EmitPrimitiveHit(Ctx, (float)NewInside.Num(), nullptr);
	}

	UE_LOG(LogIMOPDeliveryBeamV3, Verbose, TEXT("Beam Eval: %s/%s hits=%d inside=%d any=%d"),
		*GroupHandle.DeliveryId.ToString(), *PrimitiveId.ToString(), Hits.Num(), NewInside.Num(), bAny ? 1 : 0);

	InsideSet = MoveTemp(NewInside);
}

void UDeliveryDriver_BeamV3::Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* /*Group*/, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}

	bActive = false;
	InsideSet.Reset();

	EmitPrimitiveStopped(Ctx, Reason);

	UE_LOG(LogIMOPDeliveryBeamV3, Log, TEXT("Beam stopped: %s/%s inst=%d reason=%d"),
		*GroupHandle.DeliveryId.ToString(), *PrimitiveId.ToString(), GroupHandle.InstanceIndex, (int32)Reason);
}
