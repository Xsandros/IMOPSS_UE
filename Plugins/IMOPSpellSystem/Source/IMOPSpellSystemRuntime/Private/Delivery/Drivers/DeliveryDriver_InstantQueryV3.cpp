#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "Engine/EngineTypes.h" // FOverlapResult, FOverlapResult::GetActor/GetComponent
#include "Delivery/Helpers/DeliveryQueryHelpersV3.h"
#include "Delivery/Helpers/DeliveryShapeHelpersV3.h"


#include "Runtime/SpellRuntimeV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Events/SpellEventV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryInstantQueryV3, Log, All);

static USpellEventBusSubsystemV3* ResolveEventBus(const FSpellExecContextV3& Ctx)
{
	if (USpellEventBusSubsystemV3* Bus = Cast<USpellEventBusSubsystemV3>(Ctx.EventBus.Get()))
	{
		return Bus;
	}
	if (UWorld* W = Ctx.GetWorld())
	{
		return W->GetSubsystem<USpellEventBusSubsystemV3>();
	}
	return nullptr;
}

static FGuid ResolveRuntimeGuid(const FSpellExecContextV3& Ctx)
{
	if (USpellRuntimeV3* R = Cast<USpellRuntimeV3>(Ctx.Runtime.Get()))
	{
		return R->GetRuntimeGuid();
	}
	return FGuid();
}

FName UDeliveryDriver_InstantQueryV3::ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
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

void UDeliveryDriver_InstantQueryV3::SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From)
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

void UDeliveryDriver_InstantQueryV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;
	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	// Contract: Started first
	if (PrimitiveCtx.Spec.Events.bEmitStarted)
	{
		EmitPrimitiveStarted(Ctx, PrimitiveCtx.Spec.Events.ExtraTags);
	}

	// Then do the query and emit Hit/targets
	const bool bAny = EvaluateOnce(Ctx, Group, PrimitiveCtx);

	// Instant query ends immediately.
	Stop(Ctx, Group, bAny ? EDeliveryStopReasonV3::OnFirstHit : EDeliveryStopReasonV3::DurationElapsed);
}

bool UDeliveryDriver_InstantQueryV3::EvaluateOnce(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return false;
	}

	const FDeliveryPrimitiveSpecV3& Spec = PrimitiveCtx.Spec;
	const FDeliveryInstantQueryConfigV3& Q = Spec.InstantQuery;

	const FVector From = PrimitiveCtx.FinalPoseWS.GetLocation();
	const FVector Dir = PrimitiveCtx.FinalPoseWS.GetRotation().GetForwardVector().GetSafeNormal();

	const float Range = FMath::Max(0.f, Q.Range);
	const FVector To = From + Dir * Range;

	const FDeliveryDebugDrawConfigV3 DebugCfg = (Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

	// Overlay debug
	DebugDrawPrimitiveShape(World, Spec, PrimitiveCtx.FinalPoseWS, DebugCfg);
	DebugDrawBeamLine(World, Spec, From, To, DebugCfg);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_InstantQuery), /*bTraceComplex*/ false);
	FDeliveryQueryHelpersV3::BuildQueryParams(Ctx, Spec.Query, Params);

	TArray<FHitResult> Hits;
	bool bAny = false;

	// Determine query type:
	// - If Ray OR explicit LineTrace mode => line trace.
	// - Else: if Overlap => overlap at From
	// - Else => sweep From->To with collision shape
	const bool bLine =
		(Spec.Shape.Kind == EDeliveryShapeV3::Ray) ||
		(Spec.Query.Mode == EDeliveryQueryModeV3::LineTrace);

	if (bLine)
	{
		bAny = FDeliveryQueryHelpersV3::LineTraceMulti(
			World,
			Spec.Query,
			From,
			To,
			Params,
			Hits,
			TEXT("Visibility"));
	}
	else
	{
		FCollisionShape CS;
		const bool bHasCollisionShape = FDeliveryShapeHelpersV3::BuildCollisionShape(Spec.Shape, CS);

		if (Spec.Query.Mode == EDeliveryQueryModeV3::Overlap)
		{
			TArray<FOverlapResult> Overlaps;

			if (bHasCollisionShape)
			{
				bAny = FDeliveryQueryHelpersV3::OverlapMulti(
					World,
					Spec.Query,
					From,
					FQuat::Identity,
					CS,
					Params,
					Overlaps,
					TEXT("OverlapAllDynamic"));
			}
			else
			{
				// If author picked a non-collision shape for overlap, treat as "no results".
				bAny = false;
			}

			// Convert overlaps -> minimal hit list (so downstream logic stays shared)
			if (bAny)
			{
				Hits.Reserve(Overlaps.Num());
				for (const FOverlapResult& O : Overlaps)
				{
					AActor* A = O.GetActor();
					if (!A) continue;

					FHitResult& HR = Hits.AddDefaulted_GetRef();
					HR.Location = A->GetActorLocation();
					HR.ImpactPoint = HR.Location;
					HR.HitObjectHandle = FActorInstanceHandle(A);
					HR.Component = O.GetComponent();
				}
			}
		}
		else
		{
			if (bHasCollisionShape)
			{
				bAny = FDeliveryQueryHelpersV3::SweepMulti(
					World,
					Spec.Query,
					From,
					To,
					FQuat::Identity,
					CS,
					Params,
					Hits,
					TEXT("Visibility"));
			}
			else
			{
				// No usable collision shape -> nothing to do.
				bAny = false;
			}
		}
	}

	if (bAny)
{
	SortHitsDeterministic(Hits, From);
}

// Cap raw hits first (optional: keeps work bounded)
const int32 MaxHits = FMath::Max(1, Q.MaxHits);
if (Hits.Num() > MaxHits)
{
	Hits.SetNum(MaxHits);
}

// -------- NEW: Dedupe by actor (most important fix) --------
TArray<FHitResult> UniqueHits;
UniqueHits.Reserve(Hits.Num());

TSet<FObjectKey> SeenActors;
SeenActors.Reserve(Hits.Num());

auto ResolveHitActor = [](const FHitResult& H) -> AActor*
{
	if (AActor* A = H.GetActor())
	{
		return A;
	}
	if (const UPrimitiveComponent* C = H.GetComponent())
	{
		return C->GetOwner();
	}
	return nullptr;
};

for (const FHitResult& H : Hits)
{
	AActor* A = ResolveHitActor(H);
	if (!A)
	{
		continue;
	}

	const FObjectKey Key(A);
	if (SeenActors.Contains(Key))
	{
		continue;
	}

	SeenActors.Add(Key);
	UniqueHits.Add(H);
}

// If author wants single hit, enforce single *actor*
if (!Q.bMultiHit && UniqueHits.Num() > 1)
{
	UniqueHits.SetNum(1);
}

// If MaxHits should cap unique targets (more intuitive)
if (UniqueHits.Num() > MaxHits)
{
	UniqueHits.SetNum(MaxHits);
}

// -------- Debug draw should use UniqueHits (optional but consistent) --------
const FDeliveryDebugDrawConfigV3 DebugCfg2 = (Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);
if (DebugCfg2.bEnable)
{
	const float Dur = DebugCfg2.Duration;
	if (DebugCfg2.bDrawPath)
	{
		DrawDebugLine(World, From, To, FColor::Cyan, false, Dur, 0, 2.0f);
		DrawDebugDirectionalArrow(World, From, From + Dir * 120.f, 25.f, FColor::Cyan, false, Dur, 0, 2.0f);
	}
	if (DebugCfg2.bDrawHits)
	{
		for (const FHitResult& H : UniqueHits)
		{
			DrawDebugPoint(World, H.ImpactPoint, 10.f, FColor::Red, false, Dur);
		}
	}
}

// Write hits to TargetStore (optional) – use UniqueHits
const FName OutSetName = ResolveOutTargetSetName(Group, PrimitiveCtx);
if (OutSetName != NAME_None)
{
	if (USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get()))
	{
		FTargetSetV3 Out;
		for (const FHitResult& H : UniqueHits)
		{
			if (AActor* A = ResolveHitActor(H))
			{
				FTargetRefV3 R;
				R.Actor = A;
				Out.AddUnique(R);
			}
		}
		Store->Set(OutSetName, Out);
	}
}

// Emit hit magnitude based on unique actors
if (UniqueHits.Num() > 0 && Spec.Events.bEmitHit)
{
	EmitPrimitiveHit(Ctx, (float)UniqueHits.Num(), nullptr, Spec.Events.ExtraTags);
}

UE_LOG(LogIMOPDeliveryInstantQueryV3, Display, TEXT("InstantQuery hits: %d (EmitHit=%d)"),
	UniqueHits.Num(),
	(Spec.Events.bEmitHit ? 1 : 0));

return UniqueHits.Num() > 0;

}


void UDeliveryDriver_InstantQueryV3::Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason)
{
	if (!bActive) { return; }
	bActive = false;

	if (LocalCtx.Spec.Events.bEmitStopped)
	{
		EmitPrimitiveStopped(Ctx, Reason, LocalCtx.Spec.Events.ExtraTags);
	}

	UE_LOG(LogIMOPDeliveryInstantQueryV3, Log, TEXT("InstantQuery stopped: %s/%s inst=%d reason=%d"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		GroupHandle.InstanceIndex,
		(int32)Reason);
}
