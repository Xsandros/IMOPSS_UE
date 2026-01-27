#include "Delivery/DeliverySubsystemV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"

#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"
#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"
#include "Delivery/Drivers/DeliveryDriver_MoverV3.h"
#include "Delivery/Drivers/DeliveryDriver_BeamV3.h"

#include "Runtime/SpellRuntimeV3.h"
#include "Core/SpellGameplayTagsV3.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogIMOPDeliveryV3);

static uint32 HashCombineFast32(uint32 A, uint32 B)
{
	return A ^ (B + 0x9e3779b9 + (A << 6) + (A >> 2));
}

static uint32 HashName32(FName N)
{
	return ::GetTypeHash(N);
}

static FGuid ResolveRuntimeGuidFromCtx(const FSpellExecContextV3& Ctx)
{
	if (const USpellRuntimeV3* R = Cast<USpellRuntimeV3>(Ctx.Runtime.Get()))
	{
		return R->GetRuntimeGuid();
	}
	return FGuid();
}

void UDeliverySubsystemV3::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		if (USpellEventBusSubsystemV3* Bus = World->GetSubsystem<USpellEventBusSubsystemV3>())
		{
			// Subscribe to all spell events; we only react to end-ish events in OnSpellEvent.
			SpellEndSub = Bus->Subscribe(this, FGameplayTag());
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
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	TArray<FDeliveryHandleV3> Handles;
	ActiveGroups.GetKeys(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H);
		if (!Group)
		{
			continue;
		}

		const FSpellExecContextV3& Ctx = Group->CtxSnapshot;

		// Blackboard phases (enforced by BB rules)
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

	// Keep it robust even if tag list changes:
	const FString TagStr = Ev.EventTag.ToString();
	if (TagStr.Contains(TEXT("End")) || TagStr.Contains(TEXT("Ended")) || TagStr.Contains(TEXT("Stop")))
	{
		StopAllForRuntimeGuid(Ev.RuntimeGuid, EDeliveryStopReasonV3::SpellEnded);
	}
}

TObjectPtr<UDeliveryDriverBaseV3> UDeliverySubsystemV3::CreateDriverForKind(EDeliveryKindV3 Kind)
{
	switch (Kind)
	{
		case EDeliveryKindV3::InstantQuery: return NewObject<UDeliveryDriver_InstantQueryV3>(this);
		case EDeliveryKindV3::Field:       return NewObject<UDeliveryDriver_FieldV3>(this);
		case EDeliveryKindV3::Mover:       return NewObject<UDeliveryDriver_MoverV3>(this);
		case EDeliveryKindV3::Beam:        return NewObject<UDeliveryDriver_BeamV3>(this);
		default: break;
	}
	return nullptr;
}

int32 UDeliverySubsystemV3::AllocateInstanceIndex(const FGuid& RuntimeGuid, FName DeliveryId)
{
	int32& Next = NextInstanceByRuntimeAndId.FindOrAdd(RuntimeGuid).FindOrAdd(DeliveryId);
	const int32 Out = Next;
	Next++;
	return Out;
}

int32 UDeliverySubsystemV3::ComputeGroupSeed(const FGuid& RuntimeGuid, FName DeliveryId, int32 InstanceIndex) const
{
	uint32 H = 0;
	H = HashCombineFast32(H, ::GetTypeHash(RuntimeGuid));
	H = HashCombineFast32(H, HashName32(DeliveryId));
	H = HashCombineFast32(H, (uint32)InstanceIndex);
	return (int32)(H & 0x7fffffff);
}

void UDeliverySubsystemV3::EvaluateRigIfNeeded(UDeliveryGroupRuntimeV3* Group, float NowSeconds)
{
	if (!Group)
	{
		return;
	}

	const FDeliveryHandleV3& Handle = Group->GroupHandle;

	const float* LastPtr = LastRigEvalTimeByHandle.Find(Handle);
	const float Interval = FMath::Max(0.f, Group->GroupSpec.RigEvalIntervalSeconds);

	if (LastPtr && Interval > 0.f)
	{
		if ((NowSeconds - *LastPtr) < Interval)
		{
			return;
		}
	}

	// Build a delivery ctx for rig evaluation (group-level)
	FDeliveryContextV3 RigCtx;
	RigCtx.GroupHandle = Group->GroupHandle;
	RigCtx.PrimitiveId = NAME_None;
	RigCtx.PrimitiveIndex = -1;
	RigCtx.Spec = FDeliveryPrimitiveSpecV3(); // empty
	RigCtx.Caster = Group->CtxSnapshot.GetCaster();
	RigCtx.StartTimeSeconds = Group->StartTimeSeconds;
	RigCtx.Seed = Group->GroupSeed;
	RigCtx.AnchorPoseWS = FTransform::Identity;
	RigCtx.FinalPoseWS = FTransform::Identity;

	FDeliveryRigEvalResultV3 RigOut;
	FDeliveryRigEvaluatorV3::Evaluate(Group->CtxSnapshot, RigCtx, Group->GroupSpec.Rig, NowSeconds, RigOut);

	// Cache WS transforms
	Group->RigCache.RootWS = FTransform(RigOut.Root.Rotation, RigOut.Root.Location);

	Group->RigCache.EmittersWS.Reset();
	Group->RigCache.EmittersWS.Reserve(RigOut.Emitters.Num());
	Group->RigCache.EmitterNames = Group->GroupSpec.Rig.EmitterNames; // safe even if empty/mismatch

	for (const FDeliveryRigEmitterV3& E : RigOut.Emitters)
	{
		Group->RigCache.EmittersWS.Add(FTransform(E.Pose.Rotation, E.Pose.Location));
	}

	// Update all primitive anchor/final poses derived from rig
	for (auto& Pair : Group->PrimitiveCtxById)
	{
		FDeliveryContextV3& PCtx = Pair.Value;

		const FTransform AnchorWS = ResolveAnchorPoseWS(Group->CtxSnapshot, Group, PCtx);
		PCtx.AnchorPoseWS = AnchorWS;

		// For now: final pose defaults to anchor (PrimitiveMotion later can overwrite)
		PCtx.FinalPoseWS = AnchorWS;
	}

	LastRigEvalTimeByHandle.Add(Handle, NowSeconds);
}

FTransform UDeliverySubsystemV3::ResolveAnchorPoseWS(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) const
{
	if (!Group)
	{
		return FTransform::Identity;
	}

	const FDeliveryPrimitiveSpecV3& P = PrimitiveCtx.Spec;

	// Root
	if (P.Anchor.Kind == EDeliveryAnchorKindV3::Root)
	{
		return Group->RigCache.RootWS * P.Anchor.LocalOffset;
	}

	// EmitterIndex
	if (P.Anchor.Kind == EDeliveryAnchorKindV3::EmitterIndex)
	{
		const int32 Idx = P.Anchor.EmitterIndex;
		if (Group->RigCache.EmittersWS.IsValidIndex(Idx))
		{
			return Group->RigCache.EmittersWS[Idx] * P.Anchor.LocalOffset;
		}
		// fallback to root
		return Group->RigCache.RootWS * P.Anchor.LocalOffset;
	}

	// EmitterName (if present)
	if (P.Anchor.Kind == EDeliveryAnchorKindV3::EmitterName && P.Anchor.EmitterName != NAME_None)
	{
		const int32 Found = Group->RigCache.EmitterNames.IndexOfByKey(P.Anchor.EmitterName);
		if (Found != INDEX_NONE && Group->RigCache.EmittersWS.IsValidIndex(Found))
		{
			return Group->RigCache.EmittersWS[Found] * P.Anchor.LocalOffset;
		}
		return Group->RigCache.RootWS * P.Anchor.LocalOffset;
	}

	// Caster fallback
	if (AActor* Caster = Ctx.GetCaster())
	{
		return Caster->GetActorTransform() * P.Anchor.LocalOffset;
	}

	return Group->RigCache.RootWS * P.Anchor.LocalOffset;
}

bool UDeliverySubsystemV3::StartDelivery(const FSpellExecContextV3& Ctx, const FDeliverySpecV3& Spec, FDeliveryHandleV3& OutHandle)
{
	if (!GetWorld())
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
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Spec.Primitives is empty (Composite-first requires >=1). DeliveryId=%s"), *Spec.DeliveryId.ToString());
		return false;
	}

	const FGuid RuntimeGuid = ResolveRuntimeGuidFromCtx(Ctx);
	if (!RuntimeGuid.IsValid())
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Ctx.Runtime has no valid RuntimeGuid. DeliveryId=%s"), *Spec.DeliveryId.ToString());
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
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

	// Rig eval immediately
	Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Rig);
	EvaluateRigIfNeeded(Group, Now);

	// Create primitive contexts + drivers
	for (int32 i = 0; i < Spec.Primitives.Num(); ++i)
	{
		const FDeliveryPrimitiveSpecV3& PSpec = Spec.Primitives[i];
		if (PSpec.PrimitiveId == NAME_None)
		{
			UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Primitive has None PrimitiveId. DeliveryId=%s Index=%d"), *Spec.DeliveryId.ToString(), i);
			continue;
		}

		FDeliveryContextV3 PCtx;
		PCtx.GroupHandle = Handle;
		PCtx.PrimitiveId = PSpec.PrimitiveId;
		PCtx.PrimitiveIndex = i;
		PCtx.Spec = PSpec;
		PCtx.Caster = Ctx.GetCaster();
		PCtx.StartTimeSeconds = Now;
		PCtx.Seed = Group->GroupSeed ^ (int32)HashName32(PSpec.PrimitiveId);

		PCtx.AnchorPoseWS = ResolveAnchorPoseWS(Ctx, Group, PCtx);
		PCtx.FinalPoseWS = PCtx.AnchorPoseWS;

		Group->PrimitiveCtxById.Add(PSpec.PrimitiveId, PCtx);

		TObjectPtr<UDeliveryDriverBaseV3> Driver = CreateDriverForKind(PSpec.Kind);
		if (!Driver)
		{
			UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: No driver for kind=%d (%s/%s)"),
				(int32)PSpec.Kind, *Spec.DeliveryId.ToString(), *PSpec.PrimitiveId.ToString());
			continue;
		}

		Driver->GroupHandle = Handle;
		Driver->PrimitiveId = PSpec.PrimitiveId;
		Driver->bActive = true;

		Group->DriversByPrimitiveId.Add(PSpec.PrimitiveId, Driver);

		Driver->Start(Ctx, Group, PCtx);
	}

	ActiveGroups.Add(Handle, Group);
	OutHandle = Handle;

	UE_LOG(LogIMOPDeliveryV3, Log, TEXT("Delivery group started: %s inst=%d primitives=%d"),
		*Spec.DeliveryId.ToString(), InstanceIndex, Spec.Primitives.Num());

	return true;
}

void UDeliverySubsystemV3::GetActiveHandles(TArray<FDeliveryHandleV3>& Out) const
{
	Out.Reset();
	ActiveGroups.GetKeys(Out);
}

bool UDeliverySubsystemV3::StopPrimitiveInGroup(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, FName PrimitiveId, EDeliveryStopReasonV3 Reason)
{
	if (!Group)
	{
		return false;
	}

	UDeliveryDriverBaseV3* Driver = Group->DriversByPrimitiveId.FindRef(PrimitiveId);
	if (Driver && Driver->IsActive())
	{
		Driver->Stop(Ctx, Group, Reason);
	}

	Group->DriversByPrimitiveId.Remove(PrimitiveId);
	Group->PrimitiveCtxById.Remove(PrimitiveId);

	return true;
}

bool UDeliverySubsystemV3::StopGroupInternal(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason)
{
	UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(Handle);
	if (!Group)
	{
		return false;
	}

	// Stop all primitives
	TArray<FName> PrimIds;
	Group->DriversByPrimitiveId.GetKeys(PrimIds);

	for (const FName& Pid : PrimIds)
	{
		StopPrimitiveInGroup(Ctx, Group, Pid, Reason);
	}

	ActiveGroups.Remove(Handle);
	LastRigEvalTimeByHandle.Remove(Handle);

	UE_LOG(LogIMOPDeliveryV3, Log, TEXT("Delivery group stopped: %s inst=%d reason=%d"),
		*Handle.DeliveryId.ToString(), Handle.InstanceIndex, (int32)Reason);

	return true;
}

bool UDeliverySubsystemV3::StopDelivery(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason)
{
	return StopGroupInternal(Ctx, Handle, Reason);
}

bool UDeliverySubsystemV3::StopById(const FSpellExecContextV3& Ctx, FName DeliveryId, EDeliveryStopReasonV3 Reason)
{
	TArray<FDeliveryHandleV3> Handles;
	ActiveGroups.GetKeys(Handles);

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
	TArray<FDeliveryHandleV3> Handles;
	ActiveGroups.GetKeys(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.DeliveryId != DeliveryId)
		{
			continue;
		}

		if (UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H))
		{
			return StopPrimitiveInGroup(Ctx, Group, PrimitiveId, Reason);
		}
	}
	return false;
}

void UDeliverySubsystemV3::StopAllForRuntimeGuid(const FGuid& RuntimeGuid, EDeliveryStopReasonV3 Reason)
{
	TArray<FDeliveryHandleV3> Handles;
	ActiveGroups.GetKeys(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.RuntimeGuid == RuntimeGuid)
		{
			if (UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H))
			{
				const FSpellExecContextV3& Ctx = Group->CtxSnapshot;
				StopGroupInternal(Ctx, H, Reason);
			}
		}
	}
}
