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

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliverySubsystemV3 : public UTickableWorldSubsystem, public ISpellEventListenerV3
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;

	virtual void OnSpellEvent(const FSpellEventV3& Ev) override;

	bool StartDelivery(const FSpellExecContextV3& Ctx, const FDeliverySpecV3& Spec, FDeliveryHandleV3& OutHandle);
	bool StopDelivery(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason);

	bool StopById(const FSpellExecContextV3& Ctx, FName DeliveryId, EDeliveryStopReasonV3 Reason);
	bool StopByPrimitiveId(const FSpellExecContextV3& Ctx, FName DeliveryId, FName PrimitiveId, EDeliveryStopReasonV3 Reason);

	void GetActiveHandles(TArray<FDeliveryHandleV3>& Out) const;

private:
	TObjectPtr<UDeliveryDriverBaseV3> CreateDriverForKind(EDeliveryKindV3 Kind);

	int32 AllocateInstanceIndex(const FGuid& RuntimeGuid, FName DeliveryId);
	int32 ComputeGroupSeed(const FGuid& RuntimeGuid, FName DeliveryId, int32 InstanceIndex) const;

	void EvaluateRigIfNeeded(UDeliveryGroupRuntimeV3* Group, float NowSeconds);
	FTransform ResolveAnchorPoseWS(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) const;

	void StopAllForRuntimeGuid(const FGuid& RuntimeGuid, EDeliveryStopReasonV3 Reason);

	bool StopPrimitiveInGroup(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, FName PrimitiveId, EDeliveryStopReasonV3 Reason);
	bool StopGroupInternal(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason);

private:
	UPROPERTY()
	TMap<FDeliveryHandleV3, TObjectPtr<UDeliveryGroupRuntimeV3>> ActiveGroups;

	UPROPERTY()
	FSpellEventSubscriptionHandleV3 SpellEndSub;

	TMap<FGuid, TMap<FName, int32>> NextInstanceByRuntimeAndId;

	// NEW: per group last rig eval timestamp (world seconds)
	UPROPERTY()
	TMap<FDeliveryHandleV3, float> LastRigEvalTimeByHandle;
};
