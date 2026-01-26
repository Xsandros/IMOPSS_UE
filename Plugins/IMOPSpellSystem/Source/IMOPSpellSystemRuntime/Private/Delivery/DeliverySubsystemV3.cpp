#include "Delivery/DeliverySubsystemV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"

#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"
#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"
#include "Delivery/Drivers/DeliveryDriver_MoverV3.h"
#include "Delivery/Drivers/DeliveryDriver_BeamV3.h"

#include "Core/SpellGameplayTagsV3.h"
#include "Events/SpellEventBusSubsystemV3.h"

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

static USpellEventBusSubsystemV3* ResolveBus(const FSpellExecContextV3& Ctx)
{
	if (Ctx.EventBus)
	{
		return Ctx.EventBus.Get();
	}
	if (UWorld* W = Ctx.GetWorld())
	{
		return W->GetSubsystem<USpellEventBusSubsystemV3>();
	}
	return nullptr;
}

static void EmitDeliveryEvent(const FSpellExecContextV3& Ctx, const FGameplayTag& Tag, float Magnitude = 0.f)
{
	USpellEventBusSubsystemV3* Bus = ResolveBus(Ctx);
	if (!Bus || !Tag.IsValid())
	{
		return;
	}

	FSpellEventV3 Ev;
	Ev.RuntimeGuid = Ctx.RuntimeGuid;
	Ev.EventTag = Tag;
	Ev.Instigator = Ctx.Caster;
	Ev.Magnitude = Magnitude;

	Bus->Emit(Ev);
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

		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Time);
		Group->Blackboard.WriteFloat("Time.Elapsed", Now - Group->StartTimeSeconds, EDeliveryBBOwnerV3::Group);

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
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Spec.Primitives is empty (Composite-first requires >=1). DeliveryId=%s"),
			*Spec.DeliveryId.ToString());
		return false;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const int32 InstanceIndex = AllocateInstanceIndex(Ctx.RuntimeGuid, Spec.DeliveryId);

	FDeliveryHandleV3 Handle;
	Handle.RuntimeGuid = Ctx.RuntimeGuid;
	Handle.DeliveryId = Spec.DeliveryId;
	Handle.InstanceIndex = InstanceIndex;

	UDeliveryGroupRuntimeV3* Group = NewObject<UDeliveryGroupRuntimeV3>(this);
	Group->GroupHandle = Handle;
	Group->GroupSpec = Spec;
	Group->CtxSnapshot = Ctx;
	Group->StartTimeSeconds = Now;
	Group->GroupSeed = ComputeGroupSeed(Ctx.RuntimeGuid, Spec.DeliveryId, InstanceIndex);

	Group->Blackboard.InitFromSpec(Spec.BlackboardInit, Spec.OwnershipRules);

	// Evaluate rig immediately + record time
	Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Rig);
	EvaluateRigIfNeeded(Group, Now);
	LastRigEvalTimeByHandle.Add(Handle, Now);

	ActiveGroups.Add(Handle, Group);
	OutHandle = Handle;

	// ===== Emit: Group Started
	{
		const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
		EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Started, /*Magnitude=*/(float)InstanceIndex);
	}

	// ===== Spawn primitives
	for (int32 i = 0; i < Spec.Primitives.Num(); ++i)
	{
		const FDeliveryPrimitiveSpecV3& PSpec = Spec.Primitives[i];
		if (PSpec.PrimitiveId == NAME_None)
		{
			UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Primitive has None PrimitiveId. DeliveryId=%s idx=%d"),
				*Spec.DeliveryId.ToString(), i);
			continue;
		}

		FDeliveryContextV3 PCtx;
		PCtx.GroupHandle = Handle;
		PCtx.PrimitiveId = PSpec.PrimitiveId;
		PCtx.PrimitiveIndex = i;
		PCtx.Spec = PSpec;
		PCtx.Caster = Ctx.Caster;
		PCtx.StartTimeSeconds = Now;

		const uint32 Base = uint32(Group->GroupSeed);
		const uint32 Mix = HashName32(PSpec.PrimitiveId);
		PCtx.Seed = int32(HashCombineFast32(Base, Mix));

		PCtx.AnchorPoseWS = ResolveAnchorPoseWS(Ctx, Group, PCtx);
		PCtx.FinalPoseWS = PCtx.AnchorPoseWS;

		TObjectPtr<UDeliveryDriverBaseV3> Driver = CreateDriverForKind(PSpec.Kind);
		if (!Driver)
		{
			UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Failed to create driver Kind=%d PrimitiveId=%s DeliveryId=%s"),
				(int32)PSpec.Kind, *PSpec.PrimitiveId.ToString(), *Spec.DeliveryId.ToString());
			continue;
		}

		Driver->GroupHandle = Handle;
		Driver->PrimitiveId = PSpec.PrimitiveId;
		Driver->bActive = true;

		Group->PrimitiveCtxById.Add(PSpec.PrimitiveId, PCtx);
		Group->DriversByPrimitiveId.Add(PSpec.PrimitiveId, Driver);

		// Emit primitive started BEFORE driver start (so listeners can prepare)
		Driver->EmitPrimitiveStarted(Ctx);

		Driver->Start(Ctx, Group, PCtx);

		// Also emit legacy-per-primitive started tag (if you want both levels, keep)
		{
			const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
			EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Primitive_Started, /*Magnitude=*/(float)i);
		}
	}

	UE_LOG(LogIMOPDeliveryV3, Log, TEXT("StartDelivery: Group %s #%d spawned with %d primitives"),
		*Spec.DeliveryId.ToString(), InstanceIndex, Spec.Primitives.Num());

	return true;
}

bool UDeliverySubsystemV3::StopDelivery(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason)
{
	return StopGroupInternal(Ctx, Handle, Reason);
}

bool UDeliverySubsystemV3::StopById(const FSpellExecContextV3& Ctx, FName DeliveryId, EDeliveryStopReasonV3 Reason)
{
	bool bAny = false;

	TArray<FDeliveryHandleV3> Handles;
	ActiveGroups.GetKeys(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.RuntimeGuid == Ctx.RuntimeGuid && H.DeliveryId == DeliveryId)
		{
			bAny |= StopGroupInternal(Ctx, H, Reason);
		}
	}

	return bAny;
}

bool UDeliverySubsystemV3::StopByPrimitiveId(const FSpellExecContextV3& Ctx, FName DeliveryId, FName PrimitiveId, EDeliveryStopReasonV3 Reason)
{
	bool bAny = false;

	TArray<FDeliveryHandleV3> Handles;
	ActiveGroups.GetKeys(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.RuntimeGuid != Ctx.RuntimeGuid || H.DeliveryId != DeliveryId)
		{
			continue;
		}

		if (UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H))
		{
			bAny |= StopPrimitiveInGroup(Ctx, Group, PrimitiveId, Reason);
		}
	}

	return bAny;
}

void UDeliverySubsystemV3::GetActiveHandles(TArray<FDeliveryHandleV3>& Out) const
{
	Out.Reset();
	ActiveGroups.GetKeys(Out);
}

TObjectPtr<UDeliveryDriverBaseV3> UDeliverySubsystemV3::CreateDriverForKind(EDeliveryKindV3 Kind)
{
	switch (Kind)
	{
		case EDeliveryKindV3::InstantQuery: return NewObject<UDeliveryDriver_InstantQueryV3>(this);
		case EDeliveryKindV3::Field:        return NewObject<UDeliveryDriver_FieldV3>(this);
		case EDeliveryKindV3::Mover:        return NewObject<UDeliveryDriver_MoverV3>(this);
		case EDeliveryKindV3::Beam:         return NewObject<UDeliveryDriver_BeamV3>(this);
		default: break;
	}
	return nullptr;
}

int32 UDeliverySubsystemV3::AllocateInstanceIndex(const FGuid& RuntimeGuid, FName DeliveryId)
{
	int32& Next = NextInstanceByRuntimeAndId.FindOrAdd(RuntimeGuid).FindOrAdd(DeliveryId);
	const int32 Result = Next;
	Next++;
	return Result;
}

int32 UDeliverySubsystemV3::ComputeGroupSeed(const FGuid& RuntimeGuid, FName DeliveryId, int32 InstanceIndex) const
{
	uint32 H = ::GetTypeHash(RuntimeGuid);
	H = HashCombineFast32(H, HashName32(DeliveryId));
	H = HashCombineFast32(H, uint32(InstanceIndex));
	return int32(H);
}

void UDeliverySubsystemV3::EvaluateRigIfNeeded(UDeliveryGroupRuntimeV3* Group, float NowSeconds)
{
	if (!Group || !GetWorld())
	{
		return;
	}

	const FDeliverySpecV3& Spec = Group->GroupSpec;

	bool bDoEval = false;
	switch (Spec.PoseUpdatePolicy)
	{
		case EDeliveryPoseUpdatePolicyV3::EveryTick:
			bDoEval = true;
			break;

		case EDeliveryPoseUpdatePolicyV3::OnStart:
			bDoEval = (Group->RigCache.EmittersWS.Num() == 0);
			break;

		case EDeliveryPoseUpdatePolicyV3::Interval:
		default:
		{
			const float Interval = FMath::Max(0.f, Spec.PoseUpdateInterval);
			if (Interval <= 0.f)
			{
				bDoEval = true;
				break;
			}

			const float* Last = LastRigEvalTimeByHandle.Find(Group->GroupHandle);
			if (!Last)
			{
				bDoEval = true;
			}
			else
			{
				bDoEval = (NowSeconds - *Last) >= Interval;
			}
			break;
		}
	}

	if (!bDoEval)
	{
		return;
	}

	FDeliveryRigEvalResultV3 Eval;
	FDeliveryRigEvaluatorV3::Evaluate(
		GetWorld(),
		Group->CtxSnapshot.Caster.Get(),
		Spec.Attach,
		Spec.Rig,
		NowSeconds - Group->StartTimeSeconds,
		Eval);

	Group->RigCache.RootWS = Eval.RootWorld;
	Group->RigCache.EmittersWS = Eval.EmittersWorld;
	Group->RigCache.EmitterNames = Eval.EmitterNames;

	LastRigEvalTimeByHandle.Add(Group->GroupHandle, NowSeconds);

	for (auto& Pair : Group->PrimitiveCtxById)
	{
		FDeliveryContextV3& PCtx = Pair.Value;
		PCtx.AnchorPoseWS = ResolveAnchorPoseWS(Group->CtxSnapshot, Group, PCtx);
		PCtx.FinalPoseWS = PCtx.AnchorPoseWS;
	}
}

FTransform UDeliverySubsystemV3::ResolveAnchorPoseWS(const FSpellExecContextV3& /*Ctx*/, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) const
{
	if (!Group)
	{
		return FTransform::Identity;
	}

	const FDeliveryAnchorRefV3& A = PrimitiveCtx.Spec.Anchor;

	FTransform Base = Group->RigCache.RootWS;
	switch (A.Kind)
	{
		case EDeliveryAnchorRefKindV3::Root:
			Base = Group->RigCache.RootWS;
			break;

		case EDeliveryAnchorRefKindV3::EmitterIndex:
			if (Group->RigCache.EmittersWS.IsValidIndex(A.EmitterIndex))
			{
				Base = Group->RigCache.EmittersWS[A.EmitterIndex];
			}
			break;

		default:
			break;
	}

	FTransform LocalXf;
	LocalXf.SetLocation(A.LocalOffset);
	LocalXf.SetRotation(A.LocalRotation.Quaternion());
	LocalXf.SetScale3D(A.LocalScale);

	return LocalXf * Base;
}

void UDeliverySubsystemV3::StopAllForRuntimeGuid(const FGuid& RuntimeGuid, EDeliveryStopReasonV3 Reason)
{
	TArray<FDeliveryHandleV3> Handles;
	ActiveGroups.GetKeys(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.RuntimeGuid != RuntimeGuid)
		{
			continue;
		}

		if (UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H))
		{
			StopGroupInternal(Group->CtxSnapshot, H, Reason);
		}
	}
}

bool UDeliverySubsystemV3::StopPrimitiveInGroup(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, FName PrimitiveId, EDeliveryStopReasonV3 Reason)
{
	if (!Group)
	{
		return false;
	}

	UDeliveryDriverBaseV3* Driver = Group->DriversByPrimitiveId.FindRef(PrimitiveId);
	if (!Driver || !Driver->IsActive())
	{
		return false;
	}

	Driver->Stop(Ctx, Group, Reason);
	Driver->bActive = false;

	// Emit primitive stopped
	Driver->EmitPrimitiveStopped(Ctx, Reason);

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

	// Stop primitives
	for (auto& Pair : Group->DriversByPrimitiveId)
	{
		if (UDeliveryDriverBaseV3* Driver = Pair.Value)
		{
			if (Driver->IsActive())
			{
				Driver->Stop(Ctx, Group, Reason);
				Driver->bActive = false;

				Driver->EmitPrimitiveStopped(Ctx, Reason);
			}
		}
	}

	Group->DriversByPrimitiveId.Reset();
	Group->PrimitiveCtxById.Reset();

	ActiveGroups.Remove(Handle);
	LastRigEvalTimeByHandle.Remove(Handle);

	// Emit group stopped
	{
		const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
		EmitDeliveryEvent(Ctx, Tags.Event_Delivery_Stopped, /*Magnitude=*/(float)(int32)Reason);
	}

	UE_LOG(LogIMOPDeliveryV3, Log, TEXT("StopDelivery: Group %s #%d stopped (Reason=%d)"),
		*Handle.DeliveryId.ToString(), Handle.InstanceIndex, (int32)Reason);

	return true;
}
