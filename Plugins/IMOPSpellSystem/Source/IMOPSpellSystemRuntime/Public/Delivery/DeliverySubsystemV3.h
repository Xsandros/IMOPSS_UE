#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "Delivery/DeliveryContextV3.h"
#include "Actions/SpellActionExecutorV3.h" // for FSpellExecContextV3 (USTRUCT, GC-safe snapshot)
#include "DeliverySubsystemV3.generated.h"

class UDeliveryDriverBaseV3;

DECLARE_LOG_CATEGORY_EXTERN(LogIMOPDeliveryV3, Log, All);

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliverySubsystemV3 : public UTickableWorldSubsystem

{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// tick via world subsystem (we’ll use FTSTicker-style in .cpp via Tickable if needed)
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;


	bool StartDelivery(const FSpellExecContextV3& Ctx, const FDeliverySpecV3& Spec, FDeliveryHandleV3& OutHandle);
	bool StopDelivery(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason);
	bool StopById(const FSpellExecContextV3& Ctx, FName DeliveryId, EDeliveryStopReasonV3 Reason);

	void GetActiveHandles(TArray<FDeliveryHandleV3>& Out) const;

	void EmitDeliveryEvent(const FSpellExecContextV3& Ctx, const FGameplayTag& EventTag, const struct FDeliveryEventContextV3& Payload);

private:
	UPROPERTY()
	TMap<FDeliveryHandleV3, TObjectPtr<UDeliveryDriverBaseV3>> Active;

	// Ctx snapshot per active delivery (needed for ticking movers/fields; GC-safe because FSpellExecContextV3 is USTRUCT with UPROPERTYs)
	UPROPERTY()
	TMap<FDeliveryHandleV3, FSpellExecContextV3> ActiveCtx;

	
	TMap<FGuid, TMap<FName, int32>> NextInstanceByRuntimeAndId;

	TObjectPtr<UDeliveryDriverBaseV3> CreateDriverForKind(EDeliveryKindV3 Kind);

	int32 AllocateInstanceIndex(const FGuid& RuntimeGuid, FName DeliveryId);

	void StopAllForRuntime(const FSpellExecContextV3& Ctx, const FGuid& RuntimeGuid, EDeliveryStopReasonV3 Reason);
};
