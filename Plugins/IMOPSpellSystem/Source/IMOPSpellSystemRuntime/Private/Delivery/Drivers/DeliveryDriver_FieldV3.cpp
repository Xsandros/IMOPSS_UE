#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
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

void UDeliveryDriver_FieldV3::AddHitActorsUnique(const TArray<FHitResult>& Hits, TArray<TWeakObjectPtr<AActor>>& OutActors)
{
	for (const FHitResult& H : Hits)
	{
		if (AActor* A = H.GetActor())
		{
			OutActors.AddUnique(A);
		}
	}
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

	NextEvalTimeSeconds = World->GetTimeSeconds();
	LastMembers.Reset();

	// Optional immediate eval
	EvaluateOnce(Ctx, Group, PrimitiveCtx);

	UE_LOG(LogIMOPDeliveryFieldV3, Log, TEXT("Field started: %s/%s"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString());
}

void UDeliveryDriver_FieldV3::Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds)
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

	// Use live ctx (rig updates)
	const FDeliveryContextV3* LiveCtx = Group->PrimitiveCtxById.Find(PrimitiveId);
	const FDeliveryContextV3& PCtx = LiveCtx ? *LiveCtx : LocalCtx;

	const float Now = World->GetTimeSeconds();
	const float Interval = FMath::Max(0.f, PCtx.Spec.Field.TickInterval);

	if (Interval > 0.f && Now < NextEvalTimeSeconds)
	{
		return;
	}

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

	const FVector Center = PrimitiveCtx.FinalPoseWS.GetLocation();
	const FQuat Rot = PrimitiveCtx.FinalPoseWS.GetRotation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_Delivery_Field), /*bTraceComplex*/ false);
	if (Spec.Query.bIgnoreCaster)
	{
		if (AActor* Caster = Ctx.GetCaster())
		{
			Params.AddIgnoredActor(Caster);
		}
	}

	const FName Profile = (Spec.Query.CollisionProfile != NAME_None) ? Spec.Query.CollisionProfile : FName("Visibility");

	FCollisionShape CS;
	switch (Spec.Shape.Kind)
	{
		case EDeliveryShapeV3::Sphere:  CS = FCollisionShape::MakeSphere(FMath::Max(0.f, Spec.Shape.Radius)); break;
		case EDeliveryShapeV3::Capsule: CS = FCollisionShape::MakeCapsule(FMath::Max(0.f, Spec.Shape.Radius), FMath::Max(0.f, Spec.Shape.HalfHeight)); break;
		case EDeliveryShapeV3::Box:     CS = FCollisionShape::MakeBox(Spec.Shape.Extents); break;
		case EDeliveryShapeV3::Ray:
		default:
			// Field as Ray is weird; treat as small sphere overlap
			CS = FCollisionShape::MakeSphere(FMath::Max(0.f, Spec.Shape.Radius));
			break;
	}

	TArray<FHitResult> Hits;
	const bool bAny = World->OverlapMultiByProfile(Hits, Center, Rot, Profile, CS, Params);

	// Current members
	TSet<TWeakObjectPtr<AActor>> Current;
	Current.Reserve(Hits.Num());
	for (const FHitResult& H : Hits)
	{
		if (AActor* A = H.GetActor())
		{
			Current.Add(A);
		}
	}

	// Optional enter/exit bookkeeping (log-only for now)
	if (Spec.Field.bEmitEnterExit)
	{
		for (const TWeakObjectPtr<AActor>& A : Current)
		{
			if (!LastMembers.Contains(A))
			{
				if (A.IsValid())
				{
					UE_LOG(LogIMOPDeliveryFieldV3, Verbose, TEXT("Field Enter: %s/%s -> %s"),
						*GroupHandle.DeliveryId.ToString(), *PrimitiveId.ToString(), *A->GetName());
				}
			}
		}
		for (const TWeakObjectPtr<AActor>& A : LastMembers)
		{
			if (!Current.Contains(A))
			{
				if (A.IsValid())
				{
					UE_LOG(LogIMOPDeliveryFieldV3, Verbose, TEXT("Field Exit: %s/%s -> %s"),
						*GroupHandle.DeliveryId.ToString(), *PrimitiveId.ToString(), *A->GetName());
				}
			}
		}
	}

	LastMembers = MoveTemp(Current);

	// Debug
	const FDeliveryDebugDrawConfigV3 DebugCfg =
		(Spec.bOverrideDebugDraw ? Spec.DebugDrawOverride : Group->GroupSpec.DebugDrawDefaults);

	if (DebugCfg.bEnable)
	{
		const float Dur = DebugCfg.Duration;

		if (DebugCfg.bDrawShape)
		{
			switch (Spec.Shape.Kind)
			{
				case EDeliveryShapeV3::Sphere:
					DrawDebugSphere(World, Center, Spec.Shape.Radius, 16, FColor::Green, false, Dur, 0, 1.0f);
					break;
				case EDeliveryShapeV3::Capsule:
					DrawDebugCapsule(World, Center, Spec.Shape.HalfHeight, Spec.Shape.Radius, Rot, FColor::Green, false, Dur, 0, 1.0f);
					break;
				case EDeliveryShapeV3::Box:
					DrawDebugBox(World, Center, Spec.Shape.Extents, Rot, FColor::Green, false, Dur, 0, 1.0f);
					break;
				default:
					break;
			}
		}

		if (DebugCfg.bDrawHits)
		{
			for (const TWeakObjectPtr<AActor>& A : LastMembers)
			{
				if (A.IsValid())
				{
					DrawDebugPoint(World, A->GetActorLocation(), 10.f, FColor::Red, false, Dur);
				}
			}
		}
	}

	// Writeback to TargetStore
	const FName OutSetName = ResolveOutTargetSetName(Group, PrimitiveCtx);
	if (OutSetName != NAME_None)
	{
		if (USpellTargetStoreV3* Store = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get()))
		{
			FTargetSetV3 Out;
			for (const TWeakObjectPtr<AActor>& A : LastMembers)
			{
				if (A.IsValid())
				{
					FTargetRefV3 R;
					R.Actor = A.Get();
					Out.AddUnique(R);
				}
			}
			Store->Set(OutSetName, Out);
		}
	}

	return bAny;
}

void UDeliveryDriver_FieldV3::Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason)
{
	if (!bActive)
	{
		return;
	}

	bActive = false;
	LastMembers.Reset();

	UE_LOG(LogIMOPDeliveryFieldV3, Log, TEXT("Field stopped: %s/%s inst=%d reason=%d"),
		*GroupHandle.DeliveryId.ToString(),
		*PrimitiveId.ToString(),
		GroupHandle.InstanceIndex,
		(int32)Reason);
}
