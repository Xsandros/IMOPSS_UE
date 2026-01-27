#include "Delivery/DeliverySubsystemV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"

#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"
#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"
#include "Delivery/Drivers/DeliveryDriver_MoverV3.h"
#include "Delivery/Drivers/DeliveryDriver_BeamV3.h"

#include "Events/SpellEventBusSubsystemV3.h"
#include "Runtime/SpellRuntimeV3.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogIMOPDeliveryV3);

// ------------------------------------------------------------
// Local hash helpers (self-contained)
// ------------------------------------------------------------
static uint32 HashU32_Local(uint32 X)
{
	X ^= X >> 16;
	X *= 0x7feb352d;
	X ^= X >> 15;
	X *= 0x846ca68b;
	X ^= X >> 16;
	return X;
}

static uint32 HashCombineLocal(uint32 A, uint32 B)
{
	return HashU32_Local(A ^ (B + 0x9e3779b9u + (A << 6) + (A >> 2)));
}

static uint32 HashGuidStable_Local(const FGuid& G)
{
	uint32 H = 0;
	H = HashCombineLocal(H, HashU32_Local((uint32)G.A));
	H = HashCombineLocal(H, HashU32_Local((uint32)G.B));
	H = HashCombineLocal(H, HashU32_Local((uint32)G.C));
	H = HashCombineLocal(H, HashU32_Local((uint32)G.D));
	return H;
}

static uint32 HashNameStable_Local(const FName& N)
{
	// UE 5.6: GetComparisonIndex() returns FNameEntryId, stable enough for session hashing
	const uint32 A = HashU32_Local((uint32)N.GetComparisonIndex().ToUnstableInt());
	const uint32 B = HashU32_Local((uint32)N.GetNumber());
	return HashCombineLocal(A, B);
}

static uint32 HashCombineFast32(uint32 A, uint32 B)
{
	return A ^ (B + 0x9e3779b9u + (A << 6) + (A >> 2));
}

static FGuid ResolveRuntimeGuidFromCtx(const FSpellExecContextV3& Ctx)
{
	if (const USpellRuntimeV3* R = Cast<USpellRuntimeV3>(Ctx.Runtime.Get()))
	{
		return R->GetRuntimeGuid();
	}
	return FGuid();
}



// ------------------------------------------------------------

void UDeliverySubsystemV3::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		if (USpellEventBusSubsystemV3* Bus = World->GetSubsystem<USpellEventBusSubsystemV3>())
		{
			// Avoid overload ambiguity by explicitly selecting UObject overload
			SpellEndSub = Bus->Subscribe(Cast<UObject>(this), FGameplayTag());
		}
	}
}

void UDeliverySubsystemV3::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (USpellEventBusSubsystemV3* Bus = World->GetSubsystem<USpellEventBusSubsystemV3>())
		{
			Bus->Unsubscribe(SpellEndSub);
		}
	}

	ActiveGroups.Reset();
	NextInstanceByRuntimeAndId.Reset();
	LastRigEvalTimeByHandle.Reset();

	Super::Deinitialize();
}

TStatId UDeliverySubsystemV3::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDeliverySubsystemV3, STATGROUP_Tickables);
}

void UDeliverySubsystemV3::Tick(float DeltaSeconds)
{
	UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;

	TArray<FDeliveryHandleV3> Handles;
	GetActiveHandles(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H);
		if (!Group)
		{
			continue;
		}

		const FSpellExecContextV3& Ctx = Group->CtxSnapshot;

		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Time);
		Group->Blackboard.WriteFloat(TEXT("Time.Elapsed"), Now - Group->StartTimeSeconds, EDeliveryBBOwnerV3::Group);

		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Rig);
		EvaluateRigIfNeeded(Group, Now);

		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::DerivedMotion);
		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::PrimitiveMotion);
		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Query);

		for (auto& Pair : Group->DriversByPrimitiveId)
		{
			UDeliveryDriverBaseV3* Driver = Pair.Value;
			if (Driver && Driver->IsActive())
			{
				Driver->Tick(Ctx, Group, DeltaSeconds);
			}
		}

		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Stop);
		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Commit);
	}
}

void UDeliverySubsystemV3::OnSpellEvent(const FSpellEventV3& Ev)
{
	if (!Ev.RuntimeGuid.IsValid())
	{
		return;
	}

	const FString TagStr = Ev.EventTag.ToString();
	if (TagStr.Contains(TEXT("End")) || TagStr.Contains(TEXT("Ended")) || TagStr.Contains(TEXT("Stop")))
	{
		StopAllForRuntimeGuid(Ev.RuntimeGuid, EDeliveryStopReasonV3::OnEvent);
	}
}

bool UDeliverySubsystemV3::StartDelivery(const FSpellExecContextV3& Ctx, const FDeliverySpecV3& Spec, FDeliveryHandleV3& OutHandle)
{
	UWorld* W = GetWorld();
	if (!W)
	{
		return false;
	}

	if (Spec.DeliveryId == NAME_None)
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Spec.DeliveryId is None"));
		return false;
	}

	if (Spec.Primitives.Num() == 0)
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Spec.Primitives is empty (Composite-first requires >= 1). DeliveryId=%s"),
			*Spec.DeliveryId.ToString());
		return false;
	}

	const FGuid RuntimeGuid = ResolveRuntimeGuidFromCtx(Ctx);
	if (!RuntimeGuid.IsValid())
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: RuntimeGuid invalid (Ctx.Runtime missing or not USpellRuntimeV3)"));
		return false;
	}

	const float Now = W->GetTimeSeconds();
	const int32 InstanceIndex = AllocateInstanceIndex(RuntimeGuid, Spec.DeliveryId);

	FDeliveryHandleV3 Handle;
	Handle.RuntimeGuid = RuntimeGuid;
	Handle.DeliveryId = Spec.DeliveryId;
	Handle.InstanceIndex = InstanceIndex;

	UDeliveryGroupRuntimeV3* Group = NewObject<UDeliveryGroupRuntimeV3>(this);
	Group->GroupHandle = Handle;
	Group->GroupSpec = Spec;
	Group->CtxSnapshot = Ctx;
	Group->StartTimeSeconds = Now;
	Group->GroupSeed = ComputeGroupSeed(RuntimeGuid, Spec.DeliveryId, InstanceIndex);
	Group->Blackboard.InitFromSpec(Spec.BlackboardInit, Spec.OwnershipRules);

	// First rig evaluation
	Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Rig);
	EvaluateRigIfNeeded(Group, Now);
	LastRigEvalTimeByHandle.Add(Handle, Now);

	// Spawn one driver per primitive
	for (int32 i = 0; i < Spec.Primitives.Num(); ++i)
	{
		const FDeliveryPrimitiveSpecV3& PSpec = Spec.Primitives[i];
		if (PSpec.PrimitiveId == NAME_None)
		{
			UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Primitive[%d] has None PrimitiveId (DeliveryId=%s)"),
				i, *Spec.DeliveryId.ToString());
			continue;
		}

		FDeliveryContextV3 PCtx;
		PCtx.GroupHandle = Handle;
		PCtx.PrimitiveId = PSpec.PrimitiveId;
		PCtx.PrimitiveIndex = i;
		PCtx.Caster = Ctx.GetCaster();
		PCtx.StartTimeSeconds = Now;
		PCtx.Spec = PSpec;

		uint32 Seed = (uint32)Group->GroupSeed;
		Seed = HashCombineFast32(Seed, HashNameStable_Local(PSpec.PrimitiveId));
		Seed = HashCombineFast32(Seed, HashU32_Local((uint32)i));
		PCtx.Seed = (int32)Seed;

		// Pose will be resolved after all contexts exist (supports PrimitiveId anchors)
		PCtx.AnchorPoseWS = Group->RigCache.RootWS;
		const FDeliveryAnchorRefV3& A = PSpec.Anchor;
		const FTransform LocalXf(FQuat(A.LocalRotation), A.LocalOffset, A.LocalScale);
		PCtx.FinalPoseWS = LocalXf * PCtx.AnchorPoseWS;

		Group->PrimitiveCtxById.Add(PSpec.PrimitiveId, PCtx);


		UDeliveryDriverBaseV3* Driver = CreateDriverForKind(PSpec.Kind);
		if (!Driver)
		{
			UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Failed to create driver for primitive %s kind=%d"),
				*PSpec.PrimitiveId.ToString(), (int32)PSpec.Kind);
			continue;
		}

		Driver->GroupHandle = Handle;
		Driver->PrimitiveId = PSpec.PrimitiveId;
		Driver->bActive = true;

		Group->DriversByPrimitiveId.Add(PSpec.PrimitiveId, Driver);
		//Driver->Start(Ctx, Group, PCtx);
	}

	ResolveAllPrimitivePoses(Ctx, Group, Now);

	for (int32 i = 0; i < Spec.Primitives.Num(); ++i)
	{
		const FDeliveryPrimitiveSpecV3& PSpec = Spec.Primitives[i];
		if (PSpec.PrimitiveId == NAME_None) { continue; }

		FDeliveryContextV3* PCtxPtr = Group->PrimitiveCtxById.Find(PSpec.PrimitiveId);
		if (!PCtxPtr) { continue; }

		UDeliveryDriverBaseV3* Driver = CreateDriverForKind(PSpec.Kind);
		if (!Driver) { continue; }

		Driver->GroupHandle = Handle;
		Driver->PrimitiveId = PSpec.PrimitiveId;
		Driver->bActive = true;

		Group->DriversByPrimitiveId.Add(PSpec.PrimitiveId, Driver);
		Driver->Start(Ctx, Group, *PCtxPtr);
	}

	
	ActiveGroups.Add(Handle, Group);
	OutHandle = Handle;

	return true;
}

bool UDeliverySubsystemV3::StopDelivery(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason)
{
	return StopGroupInternal(Ctx, Handle, Reason);
}

bool UDeliverySubsystemV3::StopById(const FSpellExecContextV3& Ctx, FName DeliveryId, EDeliveryStopReasonV3 Reason)
{
	if (DeliveryId == NAME_None)
	{
		return false;
	}

	TArray<FDeliveryHandleV3> Handles;
	GetActiveHandles(Handles);

	bool bAny = false;
	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.DeliveryId == DeliveryId)
		{
			bAny |= StopGroupInternal(Ctx, H, Reason);
		}
	}
	return bAny;
}

bool UDeliverySubsystemV3::StopByPrimitiveId(const FSpellExecContextV3& Ctx, FName DeliveryId, FName PrimitiveId, EDeliveryStopReasonV3 Reason)
{
	if (DeliveryId == NAME_None || PrimitiveId == NAME_None)
	{
		return false;
	}

	TArray<FDeliveryHandleV3> Handles;
	GetActiveHandles(Handles);

	bool bAny = false;
	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.DeliveryId != DeliveryId)
		{
			continue;
		}

		UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H);
		if (!Group)
		{
			continue;
		}

		bAny |= StopPrimitiveInGroup(Ctx, Group, PrimitiveId, Reason);
	}
	return bAny;
}

void UDeliverySubsystemV3::GetActiveHandles(TArray<FDeliveryHandleV3>& Out) const
{
	Out.Reset();

	// Avoid TMap::GetKeys template ambiguity issues by iterating directly.
	for (auto It = ActiveGroups.CreateConstIterator(); It; ++It)
	{
		Out.Add(It.Key());
	}
}

// ------------------------------------------------------------
// Internals
// ------------------------------------------------------------

TObjectPtr<UDeliveryDriverBaseV3> UDeliverySubsystemV3::CreateDriverForKind(EDeliveryKindV3 Kind)
{
	switch (Kind)
	{
	case EDeliveryKindV3::InstantQuery:
		return NewObject<UDeliveryDriver_InstantQueryV3>(this);
	case EDeliveryKindV3::Field:
		return NewObject<UDeliveryDriver_FieldV3>(this);
	case EDeliveryKindV3::Mover:
		return NewObject<UDeliveryDriver_MoverV3>(this);
	case EDeliveryKindV3::Beam:
		return NewObject<UDeliveryDriver_BeamV3>(this);
	default:
		break;
	}
	return nullptr;
}

int32 UDeliverySubsystemV3::AllocateInstanceIndex(const FGuid& RuntimeGuid, FName DeliveryId)
{
	int32& Next = NextInstanceByRuntimeAndId.FindOrAdd(RuntimeGuid).FindOrAdd(DeliveryId);
	Next += 1;
	return Next;
}

int32 UDeliverySubsystemV3::ComputeGroupSeed(const FGuid& RuntimeGuid, FName DeliveryId, int32 InstanceIndex) const
{
	uint32 H = 0;
	H = HashCombineFast32(H, HashGuidStable_Local(RuntimeGuid));
	H = HashCombineFast32(H, HashNameStable_Local(DeliveryId));
	H = HashCombineFast32(H, HashU32_Local((uint32)InstanceIndex));
	return (int32)H;
}

void UDeliverySubsystemV3::EvaluateRigIfNeeded(UDeliveryGroupRuntimeV3* Group, float NowSeconds)
{
	if (!Group)
	{
		return;
	}

	const FDeliverySpecV3& Spec = Group->GroupSpec;

	bool bDoEval = false;
	const float Last = LastRigEvalTimeByHandle.FindRef(Group->GroupHandle);

	switch (Spec.PoseUpdatePolicy)
	{
	case EDeliveryPoseUpdatePolicyV3::EveryTick:
		bDoEval = true;
		break;
	case EDeliveryPoseUpdatePolicyV3::Interval:
	{
		const float Interval = FMath::Max(0.f, Spec.PoseUpdateInterval);
		bDoEval = (Interval <= 0.f) ? true : ((NowSeconds - Last) >= Interval);
		break;
	}
	case EDeliveryPoseUpdatePolicyV3::OnStart:
	default:
		bDoEval = !LastRigEvalTimeByHandle.Contains(Group->GroupHandle);
		break;
	}

	if (!bDoEval)
	{
		return;
	}

	FDeliveryRigEvalResultV3 RigOut;
	UDeliveryRigEvaluatorV3::Evaluate(
		GetWorld(),
		Cast<AActor>(Group->CtxSnapshot.GetCaster()),
		Spec.Attach,
		Spec.Rig,
		NowSeconds - Group->StartTimeSeconds,
		RigOut
	);

	Group->RigCache.RootWS = RigOut.RootWorld;
	Group->RigCache.EmittersWS = RigOut.EmittersWorld;
	Group->RigCache.EmitterNames = RigOut.EmitterNames;

	LastRigEvalTimeByHandle.Add(Group->GroupHandle, NowSeconds);

	ResolveAllPrimitivePoses(Group->CtxSnapshot, Group, NowSeconds);

}

bool UDeliverySubsystemV3::TryResolveAnchorPoseWS(
	const FSpellExecContextV3& /*Ctx*/,
	UDeliveryGroupRuntimeV3* Group,
	const FDeliveryContextV3& PrimitiveCtx,
	FTransform& OutAnchorPoseWS
) const
{
	OutAnchorPoseWS = FTransform::Identity;

	if (!Group)
	{
		return false;
	}

	const FDeliveryAnchorRefV3& A = PrimitiveCtx.Spec.Anchor;

	if (A.Kind == EDeliveryAnchorRefKindV3::Root)
	{
		OutAnchorPoseWS = Group->RigCache.RootWS;
		return true;
	}

	if (A.Kind == EDeliveryAnchorRefKindV3::EmitterIndex)
	{
		if (Group->RigCache.EmittersWS.IsValidIndex(A.EmitterIndex))
		{
			OutAnchorPoseWS = Group->RigCache.EmittersWS[A.EmitterIndex];
			return true;
		}

		OutAnchorPoseWS = Group->RigCache.RootWS;
		return true;
	}

	if (A.Kind == EDeliveryAnchorRefKindV3::PrimitiveId)
	{
		if (A.TargetPrimitiveId == NAME_None)
		{
			return false; // missing reference
		}

		const FDeliveryContextV3* Target = Group->PrimitiveCtxById.Find(A.TargetPrimitiveId);
		if (!Target)
		{
			return false; // not found (stopped or not created)
		}

		OutAnchorPoseWS = Target->FinalPoseWS;
		return true;
	}

	// default fallback
	OutAnchorPoseWS = Group->RigCache.RootWS;
	return true;
}

void UDeliverySubsystemV3::ResolveAllPrimitivePoses(
    const FSpellExecContextV3& Ctx,
    UDeliveryGroupRuntimeV3* Group,
    float NowSeconds
)
{
    if (!Group)
    {
        return;
    }

    // Deterministic order: sort by PrimitiveIndex (stored in ctx)
    TArray<FName> Order;
    Order.Reserve(Group->PrimitiveCtxById.Num());

    for (auto It = Group->PrimitiveCtxById.CreateConstIterator(); It; ++It)
    {
        Order.Add(It.Key());
    }

    Order.Sort([&](const FName& A, const FName& B)
    {
        const FDeliveryContextV3* CA = Group->PrimitiveCtxById.Find(A);
        const FDeliveryContextV3* CB = Group->PrimitiveCtxById.Find(B);
        const int32 IA = CA ? CA->PrimitiveIndex : 0;
        const int32 IB = CB ? CB->PrimitiveIndex : 0;
        if (IA != IB) return IA < IB;
        return A.LexicalLess(B);
    });

    // Collect StopSelf deterministically
    TArray<FName> ToStop;
    ToStop.Reserve(4);

    // N-pass so chains can resolve even if out of order
    const int32 MaxPasses = FMath::Max(1, Order.Num());

    for (int32 Pass = 0; Pass < MaxPasses; ++Pass)
    {
        bool bAnyUpdated = false;

        for (const FName& Pid : Order)
        {
            FDeliveryContextV3* PCtx = Group->PrimitiveCtxById.Find(Pid);
            if (!PCtx)
            {
                continue;
            }

            FTransform AnchorWS;
            const bool bOk = TryResolveAnchorPoseWS(Ctx, Group, *PCtx, AnchorWS);

            if (!bOk)
            {
                switch (PCtx->Spec.OnMissingAnchor)
                {
                case EDeliveryMissingAnchorPolicyV3::Freeze:
                    // Keep previous AnchorPoseWS/FinalPoseWS untouched
                    continue;

                case EDeliveryMissingAnchorPolicyV3::FallbackToRoot:
                    AnchorWS = Group->RigCache.RootWS;
                    break;

                case EDeliveryMissingAnchorPolicyV3::StopSelf:
                    if (!ToStop.Contains(Pid))
                    {
                        ToStop.Add(Pid);
                    }
                    // While waiting to stop, freeze pose (no update)
                    continue;

                default:
                    AnchorWS = Group->RigCache.RootWS;
                    break;
                }
            }

            // Apply local anchor offset
            const FDeliveryAnchorRefV3& A = PCtx->Spec.Anchor;
            const FTransform LocalXf(FQuat(A.LocalRotation), A.LocalOffset, A.LocalScale);

            PCtx->AnchorPoseWS = AnchorWS;
            PCtx->FinalPoseWS = LocalXf * PCtx->AnchorPoseWS;

            bAnyUpdated = true;
        }

        // Early out if nothing changed in this pass
        if (!bAnyUpdated)
        {
            break;
        }
    }

    // Execute StopSelf after pose resolution (stable order)
    if (ToStop.Num() > 0)
    {
        ToStop.Sort([&](const FName& A, const FName& B)
        {
            const FDeliveryContextV3* CA = Group->PrimitiveCtxById.Find(A);
            const FDeliveryContextV3* CB = Group->PrimitiveCtxById.Find(B);
            const int32 IA = CA ? CA->PrimitiveIndex : 0;
            const int32 IB = CB ? CB->PrimitiveIndex : 0;
            if (IA != IB) return IA < IB;
            return A.LexicalLess(B);
        });

        for (const FName& Pid : ToStop)
        {
            StopPrimitiveInGroup(Ctx, Group, Pid, EDeliveryStopReasonV3::Failed);
        }
    }
}


void UDeliverySubsystemV3::StopAllForRuntimeGuid(const FGuid& RuntimeGuid, EDeliveryStopReasonV3 Reason)
{
	if (!RuntimeGuid.IsValid())
	{
		return;
	}

	TArray<FDeliveryHandleV3> Handles;
	GetActiveHandles(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.RuntimeGuid == RuntimeGuid)
		{
			StopGroupInternal(FSpellExecContextV3(), H, Reason);
		}
	}
}

bool UDeliverySubsystemV3::StopPrimitiveInGroup(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, FName PrimitiveId, EDeliveryStopReasonV3 Reason)
{
	if (!Group || PrimitiveId == NAME_None)
	{
		return false;
	}

	UDeliveryDriverBaseV3* Driver = Group->DriversByPrimitiveId.FindRef(PrimitiveId);
	if (Driver && Driver->IsActive())
	{
		Driver->Stop(Ctx, Group, Reason);
		Driver->bActive = false;

		Group->DriversByPrimitiveId.Remove(PrimitiveId);
		Group->PrimitiveCtxById.Remove(PrimitiveId);

		// ===== Modell 1: Wenn keine Primitives mehr leben, Group cleanup =====
		if (Group->DriversByPrimitiveId.Num() == 0)
		{
			const FDeliveryHandleV3 Handle = Group->GroupHandle;
			LastRigEvalTimeByHandle.Remove(Handle);
			ActiveGroups.Remove(Handle);
		}

		return true;
	}

	return false;
}

bool UDeliverySubsystemV3::StopGroupInternal(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason)
{
	UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(Handle);
	if (!Group)
	{
		return false;
	}

	TArray<FName> Keys;
	Group->DriversByPrimitiveId.GetKeys(Keys);

	for (const FName& Pid : Keys)
	{
		if (UDeliveryDriverBaseV3* Driver = Group->DriversByPrimitiveId.FindRef(Pid))
		{
			if (Driver->IsActive())
			{
				Driver->Stop(Ctx, Group, Reason);
				Driver->bActive = false;
			}
		}
	}

	Group->DriversByPrimitiveId.Reset();
	Group->PrimitiveCtxById.Reset();

	LastRigEvalTimeByHandle.Remove(Handle);
	ActiveGroups.Remove(Handle);

	return true;
}

bool UDeliverySubsystemV3::StopPrimitive(const FSpellExecContextV3& Ctx, const FDeliveryPrimitiveHandleV3& PrimitiveHandle, EDeliveryStopReasonV3 Reason)
{
	if (PrimitiveHandle.PrimitiveId == NAME_None)
	{
		return false;
	}

	UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(PrimitiveHandle.Group);
	if (!Group)
	{
		return false;
	}

	return StopPrimitiveInGroup(Ctx, Group, PrimitiveHandle.PrimitiveId, Reason);
}
