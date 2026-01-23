#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/DeliveryEventContextV3.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"
#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"
#include "Delivery/Drivers/DeliveryDriver_MoverV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Events/SpellEventV3.h"
#include "Runtime/SpellRuntimeV3.h"

#include "Engine/World.h"

static void UpdateDeliveryPoseIfNeeded(const FSpellExecContextV3& Ctx, FDeliveryContextV3& Dctx, float DeltaSeconds)
{
	const bool bHasRig = !Dctx.Spec.Rig.IsEmpty();

	auto EvalPose = [&]()
	{
		if (bHasRig)
		{
			FDeliveryRigEvalResultV3 RigOut;
			FDeliveryRigEvaluatorV3::Evaluate(Ctx, Dctx, Dctx.Spec.Rig, RigOut);
			Dctx.CachedPose = FTransform(RigOut.Root.Rotation, RigOut.Root.Location);
		}
		else
		{
			// Use Attach
			// If you already have a shared ResolveAttachTransform helper, use that.
			// Fallback: world/caster handled by drivers already; but here we prefer unified cache.
			const FTransform Xf = FTransform::Identity;
			Dctx.CachedPose = Xf;
		}
	};

	switch (Dctx.Spec.PoseUpdatePolicy)
	{
	case EDeliveryPoseUpdatePolicyV3::OnStart:
		// only update if not set yet
		if (Dctx.CachedPose.Equals(FTransform::Identity))
		{
			EvalPose();
		}
		break;

	case EDeliveryPoseUpdatePolicyV3::EveryTick:
		EvalPose();
		break;

	case EDeliveryPoseUpdatePolicyV3::Interval:
		{
			const float Interval = (Dctx.Spec.PoseUpdateInterval > 0.f) ? Dctx.Spec.PoseUpdateInterval : 0.f;
			if (Interval <= 0.f)
			{
				// treat as EveryTick if interval unspecified
				EvalPose();
			}
			else
			{
				Dctx.PoseAccum += DeltaSeconds;
				if (Dctx.PoseAccum >= Interval || Dctx.CachedPose.Equals(FTransform::Identity))
				{
					Dctx.PoseAccum = 0.f;
					EvalPose();
				}
			}
			break;
		}
	default:
		break;
	}
}


DEFINE_LOG_CATEGORY(LogIMOPDeliveryV3);

void UDeliverySubsystemV3::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogIMOPDeliveryV3, Log, TEXT("DeliverySubsystemV3 Initialize (World=%s)"), *GetNameSafe(GetWorld()));
	
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			if (USpellEventBusSubsystemV3* Bus = GI->GetSubsystem<USpellEventBusSubsystemV3>())
			{
				const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

				FSpellTriggerMatcherV3 M;
				M.bUseExactTag = true;
				M.ExactTag = Tags.Event_Spell_End;

				SpellEndSub = Bus->Subscribe(this, M);
				UE_LOG(LogIMOPDeliveryV3, Log, TEXT("DeliverySubsystem: subscribed Spell.End (handle=%d)"), SpellEndSub.Id);
			}
		}
	}

}

void UDeliverySubsystemV3::Deinitialize()
{
	UE_LOG(LogIMOPDeliveryV3, Log, TEXT("DeliverySubsystemV3 Deinitialize: stopping %d active deliveries"), Active.Num());
	Active.Empty();
	ActiveCtx.Empty();
	NextInstanceByRuntimeAndId.Empty();
	Super::Deinitialize();
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			if (USpellEventBusSubsystemV3* Bus = GI->GetSubsystem<USpellEventBusSubsystemV3>())
			{
				if (SpellEndSub.IsValid())
				{
					Bus->Unsubscribe(SpellEndSub);
				}
				for (auto& It : StopTagSubs)
				{
					Bus->Unsubscribe(It.Value);
				}
			}
		}
	}

	StopTagSubs.Empty();
	StopByTag.Empty();
	ActiveDelivery.Empty();

}

TStatId UDeliverySubsystemV3::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDeliverySubsystemV3, STATGROUP_Tickables);
}

void UDeliverySubsystemV3::Tick(float DeltaSeconds)
{
	// Robust ticking + cleanup invalid drivers
	TArray<FDeliveryHandleV3> ToRemove;

	for (auto& It : Active)
	{
		const FDeliveryHandleV3 Handle = It.Key;
		UDeliveryDriverBaseV3* Driver = It.Value.Get();

		if (!Driver || !Driver->IsActive())
		{
			ToRemove.Add(Handle);
			continue;
		}

		FSpellExecContextV3* CtxPtr = ActiveCtx.Find(Handle);
		if (!CtxPtr)
		{
			UE_LOG(LogIMOPDeliveryV3, Warning,
				TEXT("DeliverySubsystem Tick: missing ActiveCtx for Id=%s Inst=%d (removing)"),
				*Handle.DeliveryId.ToString(), Handle.InstanceIndex);
			ToRemove.Add(Handle);
			continue;
		}

		Driver->Tick(*CtxPtr, DeltaSeconds);

		// driver may have stopped itself during tick
		if (!Driver->IsActive())
		{
			ToRemove.Add(Handle);
		}
	}

	for (const FDeliveryHandleV3& H : ToRemove)
	{
		Active.Remove(H); 
		ActiveCtx.Remove(H); 
		ActiveDelivery.Remove(H);
	}

}


int32 UDeliverySubsystemV3::AllocateInstanceIndex(const FGuid& RuntimeGuid, FName DeliveryId)
{
	int32& Next = NextInstanceByRuntimeAndId.FindOrAdd(RuntimeGuid).FindOrAdd(DeliveryId);
	Next += 1;
	return Next;
}

TObjectPtr<UDeliveryDriverBaseV3> UDeliverySubsystemV3::CreateDriverForKind(EDeliveryKindV3 Kind)
{
	switch (Kind)
	{
	case EDeliveryKindV3::InstantQuery:
		return NewObject<UDeliveryDriver_InstantQueryV3>(this);
	case EDeliveryKindV3::Field:
		return NewObject<UDeliveryDriver_FieldV3>(this);
	case EDeliveryKindV3::Mover: return NewObject<UDeliveryDriver_MoverV3>(this);


	default:
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("CreateDriverForKind: Kind %d not implemented yet"), (int32)Kind);
		return nullptr;
	}
}

bool UDeliverySubsystemV3::StartDelivery(const FSpellExecContextV3& Ctx, const FDeliverySpecV3& Spec, FDeliveryHandleV3& OutHandle)
{
	OutHandle = FDeliveryHandleV3();

	if (!Ctx.Runtime)
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery failed: Ctx.Runtime is null"));
		return false;
	}
	if (!Ctx.EventBus)
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery failed: Ctx.EventBus is null"));
		return false;
	}
	if (Spec.DeliveryId == NAME_None)
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery failed: Spec.DeliveryId is None"));
		return false;
	}

	const FGuid RuntimeGuid = Ctx.Runtime->GetRuntimeGuid();

	// Evaluate rig once to see if we have multi-emitters (OrbitSampler etc.)
	int32 EmitterCount = 0;
	if (!Spec.Rig.IsEmpty())
	{
		FDeliveryContextV3 Temp;
		Temp.Handle.RuntimeGuid = RuntimeGuid;
		Temp.Handle.DeliveryId = Spec.DeliveryId;
		Temp.Handle.InstanceIndex = 0;
		Temp.Spec = Spec;
		Temp.Caster = Ctx.Caster;
		Temp.StartTime = Ctx.GetWorld() ? Ctx.GetWorld()->GetTimeSeconds() : 0.f;
		Temp.Seed = HashCombine(Ctx.Seed, GetTypeHash(Spec.DeliveryId));
		Temp.EmitterIndex = INDEX_NONE;

		FDeliveryRigEvalResultV3 RigOut;
		FDeliveryRigEvaluatorV3::Evaluate(Ctx, Temp, Spec.Rig, RigOut);
		EmitterCount = RigOut.Emitters.Num();
	}

	// Spawn either 1 instance (no emitters) or N instances (one per emitter)
	const int32 SpawnCount = (EmitterCount > 0) ? EmitterCount : 1;
	UE_LOG(LogIMOPDeliveryV3, Log, TEXT("StartDelivery: Kind=%d Id=%s Runtime=%s Emitters=%d -> Spawn=%d"),
		(int32)Spec.Kind, *Spec.DeliveryId.ToString(), *RuntimeGuid.ToString(), EmitterCount, SpawnCount);

	FDeliveryHandleV3 FirstHandle;

	for (int32 i = 0; i < SpawnCount; i++)
	{
		const int32 InstanceIndex = AllocateInstanceIndex(RuntimeGuid, Spec.DeliveryId);

		FDeliveryHandleV3 Handle;
		Handle.RuntimeGuid = RuntimeGuid;
		Handle.DeliveryId = Spec.DeliveryId;
		Handle.InstanceIndex = InstanceIndex;

		const int32 Seed = HashCombine(Ctx.Seed, HashCombine(GetTypeHash(Spec.DeliveryId), HashCombine(InstanceIndex, i)));

		FDeliveryContextV3 Dctx;
		Dctx.Handle = Handle;
		Dctx.Spec = Spec;
		Dctx.Caster = Ctx.Caster;
		Dctx.StartTime = Ctx.GetWorld() ? Ctx.GetWorld()->GetTimeSeconds() : 0.f;
		Dctx.Seed = Seed;

		// If rig had emitters, bind this instance to a specific emitter index
		Dctx.EmitterIndex = (EmitterCount > 0) ? i : INDEX_NONE;

		UDeliveryDriverBaseV3* Driver = CreateDriverForKind(Spec.Kind).Get();
		if (!Driver)
		{
			UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery failed: driver create failed (Kind=%d)"), (int32)Spec.Kind);
			// Stop already started instances for robustness
			if (FirstHandle.IsValid())
			{
				StopById(Ctx, Spec.DeliveryId, EDeliveryStopReasonV3::Failed);
			}
			return false;
		}

		Active.Add(Handle, Driver);
		ActiveCtx.Add(Handle, Ctx);
		// Optional: if you maintain ActiveDelivery map, keep it in sync (safe even if map exists)
		ActiveDelivery.Add(Handle, Dctx);

		UE_LOG(LogIMOPDeliveryV3, Verbose, TEXT("StartDelivery Instance: Id=%s Inst=%d Seed=%d EmitterIndex=%d"),
			*Handle.DeliveryId.ToString(), Handle.InstanceIndex, Seed, Dctx.EmitterIndex);

		Driver->Start(Ctx, Dctx);

		if (!FirstHandle.IsValid())
		{
			FirstHandle = Handle;
		}
	}

	OutHandle = FirstHandle;
	return true;
}


bool UDeliverySubsystemV3::StopDelivery(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason)
{
	if (UDeliveryDriverBaseV3* Driver = Active.FindRef(Handle).Get())
	{
		UE_LOG(LogIMOPDeliveryV3, Log, TEXT("StopDelivery: %s Inst=%d Reason=%d"), *Handle.DeliveryId.ToString(), Handle.InstanceIndex, (int32)Reason);
		Driver->Stop(Ctx, Reason);
		Active.Remove(Handle);
		// Cleanup stop-by-tag mapping
		if (const FDeliveryContextV3* D = ActiveDelivery.Find(Handle))
		{
			const FGameplayTag StopTag = D->Spec.StopPolicy.StopOnEventTag;
			if (StopTag.IsValid())
			{
				if (TArray<FDeliveryHandleV3>* Arr = StopByTag.Find(StopTag))
				{
					Arr->RemoveAll([&](const FDeliveryHandleV3& H) { return H == Handle; });
				}
			}
		}
		ActiveDelivery.Remove(Handle);

		return true;

	}
	return false;
}

bool UDeliverySubsystemV3::StopById(const FSpellExecContextV3& Ctx, FName DeliveryId, EDeliveryStopReasonV3 Reason)
{
	if (!Ctx.Runtime || DeliveryId == NAME_None)
	{
		return false;
	}

	const FGuid RuntimeGuid = Ctx.Runtime->GetRuntimeGuid();

	TArray<FDeliveryHandleV3> Matches;
	for (const auto& It : Active)
	{
		const FDeliveryHandleV3& H = It.Key;
		if (H.RuntimeGuid == RuntimeGuid && H.DeliveryId == DeliveryId)
		{
			Matches.Add(H);
		}
	}

	for (const FDeliveryHandleV3& H : Matches)
	{
		StopDelivery(Ctx, H, Reason);
	}

	return Matches.Num() > 0;
}

void UDeliverySubsystemV3::GetActiveHandles(TArray<FDeliveryHandleV3>& Out) const
{
	Out.Reset();
	Active.GetKeys(Out);
}

void UDeliverySubsystemV3::EmitDeliveryEvent(const FSpellExecContextV3& Ctx, const FGameplayTag& EventTag, const FDeliveryEventContextV3& Payload)
{
	if (!Ctx.EventBus)
	{
		return;
	}

	FSpellEventV3 Ev;
	Ev.EventTag = EventTag;
	Ev.Caster = Ctx.Caster;
	Ev.Sender = this;
	Ev.FrameNumber = GFrameNumber;
	Ev.TimeSeconds = Ctx.GetWorld() ? Ctx.GetWorld()->GetTimeSeconds() : 0.f;
	Ev.RuntimeGuid = Ctx.Runtime ? Ctx.Runtime->GetRuntimeGuid() : FGuid();

	Ev.Data.InitializeAs<FDeliveryEventContextV3>(Payload);


	UE_LOG(LogIMOPDeliveryV3, Verbose, TEXT("EmitDeliveryEvent: %s Hits=%d Id=%s Inst=%d"),
		*EventTag.ToString(),
		Payload.Hits.Num(),
		*Payload.Handle.DeliveryId.ToString(),
		Payload.Handle.InstanceIndex);

	Ctx.EventBus->Emit(Ev);
}

void UDeliverySubsystemV3::StopAllForRuntime(const FSpellExecContextV3& Ctx, const FGuid& RuntimeGuid, EDeliveryStopReasonV3 Reason)
{
	TArray<FDeliveryHandleV3> Matches;
	for (const auto& It : Active)
	{
		const FDeliveryHandleV3& H = It.Key;
		if (H.RuntimeGuid == RuntimeGuid)
		{
			Matches.Add(H);
		}
	}

	for (const FDeliveryHandleV3& H : Matches)
	{
		StopDelivery(Ctx, H, Reason);
	}

	UE_LOG(LogIMOPDeliveryV3, Log, TEXT("StopAllForRuntime: runtime=%s stopped=%d reason=%d"),
		*RuntimeGuid.ToString(), Matches.Num(), (int32)Reason);
}

void UDeliverySubsystemV3::OnSpellEvent(const FSpellEventV3& Ev)
{
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

	// Spell.End => stop all deliveries for that runtime
	if (Ev.EventTag == Tags.Event_Spell_End)
	{
		// We need an exec ctx to stop drivers. Use stored ActiveCtx of any handle of that runtime.
		for (const auto& It : ActiveCtx)
		{
			const FDeliveryHandleV3& H = It.Key;
			if (H.RuntimeGuid == Ev.RuntimeGuid)
			{
				const FSpellExecContextV3& Ctx = It.Value;
				UE_LOG(LogIMOPDeliveryV3, Log, TEXT("OnSpellEvent: Spell.End runtime=%s -> stopping deliveries"), *Ev.RuntimeGuid.ToString());
				StopAllForRuntime(Ctx, Ev.RuntimeGuid, EDeliveryStopReasonV3::OwnerDestroyed);
				break;
			}
		}
		return;
	}

	// StopOnEventTag routing
	if (TArray<FDeliveryHandleV3>* Arr = StopByTag.Find(Ev.EventTag))
	{
		// Snapshot because StopDelivery mutates maps
		const TArray<FDeliveryHandleV3> Handles = *Arr;

		int32 Stopped = 0;
		for (const FDeliveryHandleV3& H : Handles)
		{
			if (H.RuntimeGuid != Ev.RuntimeGuid)
			{
				continue;
			}

			if (const FSpellExecContextV3* CtxPtr = ActiveCtx.Find(H))
			{
				if (StopDelivery(*CtxPtr, H, EDeliveryStopReasonV3::OnEvent))
				{
					Stopped++;
				}
			}
		}

		if (Stopped > 0)
		{
			UE_LOG(LogIMOPDeliveryV3, Log, TEXT("OnSpellEvent: StopOnEventTag=%s runtime=%s stopped=%d"),
				*Ev.EventTag.ToString(), *Ev.RuntimeGuid.ToString(), Stopped);
		}
	}
}
