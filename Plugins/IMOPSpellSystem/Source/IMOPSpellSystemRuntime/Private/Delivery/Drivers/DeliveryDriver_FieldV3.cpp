#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "Actions/SpellActionExecutorV3.h" // FSpellExecContextV3::GetWorld / GetCaster
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionShape.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryFieldV3, Log, All);

static FCollisionShape MakeCollisionShape_Field(const FDeliveryShapeV3& Shape)
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
			// Ray is not meaningful as a field volume; use tiny sphere.
			return FCollisionShape::MakeSphere(1.f);
	}
}

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

void UDeliveryDriver_FieldV3::SortActorsDeterministic(TArray<AActor*>& Actors)
{
	Actors.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetFName().LexicalLess(B.GetFName());
	});
}

bool UDeliveryDriver_FieldV3::DoOverlap(
	UWorld* World,
	const FVector& Origin,
	const FQuat& Rot,
	const FDeliveryShapeV3& Shape,
	FName Profile,
	const FCollisionQueryParams& Params,
	TArray<FHitResult>& OutHits) const
{
	if (!World)
	{
		return false;
	}

	const FCollisionShape CS = MakeCollisionShape_Field(Shape);
	return World->OverlapMultiByProfile(OutHits, Origin, Rot, Profile, CS, Params);
}

void UDeliveryDriver_FieldV3::Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	LocalCtx = PrimitiveCtx;

	GroupHandle = PrimitiveCtx.GroupHandle;
	PrimitiveId = PrimitiveCtx.PrimitiveId;
	bActive = true;

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPDeliveryFieldV3, Error, TEXT("Field Start failed: World is null"));
		Stop(Ctx, Group, EDeliveryStopReasonV3::Failed);
		return;
	}

	// Schedule first eval immediately
	NextEvalTimeSeconds = World->GetTimeSeconds();

	// Optional: one immediate eval on start (feels good for responsiveness)
	EvaluateOnce(Ctx, Group, PrimitiveCtx);

	UE_LOG(LogIMOPDeliveryFieldV3, Log, TEXT("Field started: %s/%s"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString());
}

void UDeliveryDriver_FieldV3::Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds)
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

	// Fetch latest primitive ctx from group (so moving rig updates are used)
	const FDeliveryContextV3* LiveCtx = Group->PrimitiveCtxById.Find(PrimitiveId);
	const FDeliveryContextV3& PCtx = LiveCtx ? *LiveCtx : LocalCtx;

	const float Now = World->GetTimeSeconds();
	const float Interval = FMath::Max(0.f, PCtx.Spec.Field.TickInterval);

	// Interval==0 => every tick
	if (Interval > 0.f && Now < NextEvalTimeSeconds)
	{
		return;
	}

	// Advance schedule (stable)
	NextEvalTimeSeconds = (Interval > 0.f) ? (Now + Interval) : Now;

	EvaluateOnce(Ctx, Group, PCtx);
}

bool UDeliveryDriver_FieldV3::EvaluateOnce(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
{
	UWorld* World = Ctx.GetWorld();
	if (!World || !Group)
	{
		return false;
	}

	const FDeliveryPrimitiveSpecV3& Spec = PrimitiveCtx.Spec;

	// We treat Field as an overlap volume. If author set Sweep/LineTrace, we still overlap for now.
	if (Spec.Query.Mode != EDeliveryQueryModeV3::Overlap)
	{
		UE_LOG(LogIMOPDeliveryFieldV3, Warning, TEXT("Field %s/%s: Query.Mode != Overlap; forcing Overlap for Field."),
			*GroupHandle.DeliveryId.ToString(), *PrimitiveId.ToString());
	}

	const FVector Origin = PrimitiveCtx.FinalPoseWS.GetLocation();
	const FQuat Rot = PrimitiveCtx.FinalPoseWS.GetRotation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Field), /*bTraceComplex*/ false);
	if (Spec.Query.bIgnoreCaster)
	{
		if (AActor* Caster = Ctx.GetCaster())
		{
			Params.AddIgnoredActor(Caster);
		}
	}

	const FName Profile = (Spec.Query.CollisionProfile != NAME_None) ? Spec.Query.CollisionProfile : FName("OverlapAll");

	TArray<FHitResult> Hits;
	const bool bAny = DoOverlap(World, Origin, Rot, Spec.Shape, Profile, Params, Hits);

	// Build actor list (unique)
	TSet<TWeakObjectPtr<AActor>> NewMembers;
	TArray<AActor*> NewActors;
	NewActors.Reserve(Hits.Num());

	for (const FHitResult& H : Hits)
	{
		AActor* A = H.GetActor();
		if (!A)
		{
			continue;
		}
		if (!NewMembers.Contains(A))
		{
			NewMembers.Add(A);
			NewActors.Add(A);
		}
	}

	SortActorsDeterministic(NewActors);

	// Debug draw
	const FDeliveryDebugDrawConfigV3 DebugCfg =
		(Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

	if (DebugCfg.bEnable && DebugCfg.bDrawShape)
	{
		const float Dur = DebugCfg.Duration;
		switch (Spec.Shape.Kind)
		{
			case EDeliveryShapeV3::Sphere:
				DrawDebugSphere(World, Origin, Spec.Shape.Radius, 16, FColor::Green, false, Dur, 0, 1.f);
				break;
			case EDeliveryShapeV3::Capsule:
				DrawDebugCapsule(World, Origin, Spec.Shape.HalfHeight, Spec.Shape.Radius, Rot, FColor::Green, false, Dur, 0, 1.f);
				break;
			case EDeliveryShapeV3::Box:
				DrawDebugBox(World, Origin, Spec.Shape.Extents, Rot, FColor::Green, false, Dur, 0, 1.f);
				break;
			case EDeliveryShapeV3::Ray:
			default:
				DrawDebugPoint(World, Origin, 10.f, FColor::Green, false, Dur);
				break;
		}
	}

	if (DebugCfg.bEnable && DebugCfg.bDrawHits)
	{
		for (AActor* A : NewActors)
		{
			DrawDebugPoint(World, A->GetActorLocation(), 8.f, FColor::Red, false, DebugCfg.Duration);
		}
	}

	// Membership diff (for future events)
	// Enter = in New but not in Current; Exit = in Current but not in New
	if (Spec.Field.bEmitEnterExit)
	{
		for (const TWeakObjectPtr<AActor>& W : NewMembers)
		{
			if (!CurrentMembers.Contains(W))
			{
				// Enter (future event hook)
			}
		}
		for (const TWeakObjectPtr<AActor>& W : CurrentMembers)
		{
			if (!NewMembers.Contains(W))
			{
				// Exit (future event hook)
			}
		}
	}

	CurrentMembers = MoveTemp(NewMembers);

	// Optional: write current set into TargetStore
	const FName OutSetName = ResolveOutTargetSetName(Group, PrimitiveCtx);
	if (OutSetName != NAME_None)
	{
		if (USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get()))
		{
			FTargetSetV3 OutSet;
			OutSet.Targets.Reserve(NewActors.Num());

			for (AActor* A : NewActors)
			{
				FTargetRefV3 R;
				R.Actor = A;
				OutSet.AddUnique(R);
			}

			Store->Set(OutSetName, OutSet);
		}
	}

	UE_LOG(LogIMOPDeliveryFieldV3, Verbose, TEXT("Field tick: %s/%s members=%d any=%d"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		CurrentMembers.Num(),
		bAny ? 1 : 0);

	return bAny;
}

void UDeliveryDriver_FieldV3::Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}

	bActive = false;
	CurrentMembers.Reset();

	UE_LOG(LogIMOPDeliveryFieldV3, Log, TEXT("Field stopped: %s/%s inst=%d reason=%d"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		GroupHandle.InstanceIndex,
		(int32)Reason);
}
