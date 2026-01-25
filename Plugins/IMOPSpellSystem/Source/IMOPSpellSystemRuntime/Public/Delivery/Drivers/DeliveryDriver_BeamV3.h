#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "Delivery/DeliveryTypesV3.h"
#include "DeliveryDriver_BeamV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriver_BeamV3 : public UDeliveryDriverBaseV3
{
	GENERATED_BODY()

public:
	virtual void Start(const FSpellExecContextV3& Ctx, const FDeliveryContextV3& InDeliveryCtx) override;
	virtual void Tick(const FSpellExecContextV3& Ctx, float DeltaSeconds) override;
	virtual void Stop(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason) override;

	virtual FDeliveryHandleV3 GetHandle() const override { return DeliveryCtx.Handle; }
	virtual bool IsActive() const override { return bActive; }

private:
	// --- Attach / pose helpers ---
	FTransform ResolveAttachTransform(const FSpellExecContextV3& Ctx) const;
	bool ResolveBeamOriginAndDir(const FSpellExecContextV3& Ctx, FVector& OutOrigin, FVector& OutDir) const;

	// --- Queries ---
	void EvaluateBeam(const FSpellExecContextV3& Ctx, const FVector& Origin, const FVector& Dir, float Range);

	// --- Deterministic / inside tracking ---
	static void BuildSortedActorsDeterministic(const TSet<TWeakObjectPtr<AActor>>& In, TArray<AActor*>& Out);
	static void BuildSortedActorsDeterministicFromHits(const TArray<FHitResult>& InHits, TArray<AActor*>& OutActors, const FVector& From);

	void EmitStarted(const FSpellExecContextV3& Ctx);
	void EmitStopped(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason);
	void EmitEnterExitStayTick(
		const FSpellExecContextV3& Ctx,
		const TArray<AActor*>& EnterActors,
		const TArray<AActor*>& ExitActors,
		const TArray<AActor*>& StayActors,
		bool bEmitTick);

	void WritebackTargets(const FSpellExecContextV3& Ctx, const TArray<AActor*>& Actors) const;

	// --- State ---
	UPROPERTY()
	FDeliveryContextV3 DeliveryCtx;

	bool bActive = false;

	float TimeSinceStart = 0.f;
	float TimeSinceLastEval = 0.f;

	// persistent set for Enter/Exit/Stay like Field
	TSet<TWeakObjectPtr<AActor>> InsideSet;
};
