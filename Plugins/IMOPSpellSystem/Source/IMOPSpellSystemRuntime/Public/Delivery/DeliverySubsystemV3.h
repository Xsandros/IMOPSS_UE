#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "Delivery/DeliveryContextV3.h"

#include "Events/SpellEventListenerV3.h"
#include "Events/SpellEventBusSubsystemV3.h"

#include "DeliverySubsystemV3.generated.h"

class UDeliveryDriverBaseV3;
class UDeliveryGroupRuntimeV3;

DECLARE_LOG_CATEGORY_EXTERN(LogIMOPDeliveryV3, Log, All);

/**
 * Composite-first Delivery Subsystem (Phase 4B):
 * - StartDelivery spawns a GROUP runtime.
 * - Each Primitive in Spec.Primitives spawns its own driver (heterogeneous).
 * - Group owns Blackboard (phase/owner enforcement) + Rig cache.
 * - Stop can target group handle, delivery id, or a specific primitive id.
 *
 * NOTE (compile-first): We do NOT depend on SpellEventV3 carrying a delivery payload.
 *       Drivers/subsystem can emit plain SpellEvents by tag later without requiring a payload field.
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliverySubsystemV3 : public UTickableWorldSubsystem, public ISpellEventListenerV3
{
	GENERATED_BODY()

public:
	// USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Tick
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;

	// ISpellEventListenerV3
	virtual void OnSpellEvent(const FSpellEventV3& Ev) override;

	// API
	bool StartDelivery(const FSpellExecContextV3& Ctx, const FDeliverySpecV3& Spec, FDeliveryHandleV3& OutHandle);
	bool StopDelivery(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason);

	// Stops ALL groups matching DeliveryId (within same RuntimeGuid).
	bool StopById(const FSpellExecContextV3& Ctx, FName DeliveryId, EDeliveryStopReasonV3 Reason);

	// Stops ONLY a primitive inside matching groups (DeliveryId required for routing).
	bool StopByPrimitiveId(const FSpellExecContextV3& Ctx, FName DeliveryId, FName PrimitiveId, EDeliveryStopReasonV3 Reason);

	void GetActiveHandles(TArray<FDeliveryHandleV3>& Out) const;

private:
	// ===== Runtime helpers =====
	TObjectPtr<UDeliveryDriverBaseV3> CreateDriverForKind(EDeliveryKindV3 Kind);

	int32 AllocateInstanceIndex(const FGuid& RuntimeGuid, FName DeliveryId);
	int32 ComputeGroupSeed(const FGuid& RuntimeGuid, FName DeliveryId, int32 InstanceIndex) const;

	// Rig/anchor resolve (Etappe 2: Root + EmitterIndex)
	void EvaluateRigIfNeeded(UDeliveryGroupRuntimeV3* Group, float NowSeconds);
	FTransform ResolveAnchorPoseWS(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) const;

	// Stop all groups belonging to a RuntimeGuid (uses each group's stored snapshot context)
	void StopAllForRuntimeGuid(const FGuid& RuntimeGuid, EDeliveryStopReasonV3 Reason);

	// Stop one primitive inside a group; returns whether something was stopped.
	bool StopPrimitiveInGroup(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, FName PrimitiveId, EDeliveryStopReasonV3 Reason);

	// Stop the group and remove it from ActiveGroups.
	bool StopGroupInternal(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason);

private:
	UPROPERTY()
	TMap<FDeliveryHandleV3, TObjectPtr<UDeliveryGroupRuntimeV3>> ActiveGroups;

	UPROPERTY()
	FSpellEventSubscriptionHandleV3 SpellEndSub;

	// Next instance index per (RuntimeGuid, DeliveryId)
	TMap<FGuid, TMap<FName, int32>> NextInstanceByRuntimeAndId;
};
