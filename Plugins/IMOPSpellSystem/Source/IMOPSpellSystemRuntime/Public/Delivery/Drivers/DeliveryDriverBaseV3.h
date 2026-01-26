#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliveryContextV3.h"

#include "Events/SpellEventV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Core/SpellGameplayTagsV3.h"

#include "DeliveryDriverBaseV3.generated.h"

struct FSpellExecContextV3;
class UDeliveryGroupRuntimeV3;

/**
 * Base class for all Delivery drivers (Composite-first).
 *
 * Lifecycle:
 * - Start(Ctx, Group, PrimitiveCtx)
 * - Tick(Ctx, Group, DeltaSeconds) (optional)
 * - Stop(Ctx, Group, Reason)
 *
 * Notes:
 * - Drivers are owned by DeliverySubsystemV3 / GroupRuntime and are server-authoritative.
 * - Drivers keep minimal state + can read/write Group->Blackboard and Group->PrimitiveCtxById.
 * - Event emission helpers live here so drivers stay consistent.
 */
UCLASS(Abstract)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriverBaseV3 : public UObject
{
	GENERATED_BODY()

public:
	// Set by subsystem before Start()
	UPROPERTY()
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY()
	FName PrimitiveId = NAME_None;

	UPROPERTY()
	bool bActive = false;

public:
	virtual void Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx)
		PURE_VIRTUAL(UDeliveryDriverBaseV3::Start, );

	virtual void Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds) {}

	virtual void Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason)
		PURE_VIRTUAL(UDeliveryDriverBaseV3::Stop, );

	bool IsActive() const { return bActive; }

protected:
	USpellEventBusSubsystemV3* GetBus(const FSpellExecContextV3& Ctx) const;

	void EmitPrimitiveEvent(
		const FSpellExecContextV3& Ctx,
		const FGameplayTag& EventTag,
		float Magnitude = 0.f,
		const FGameplayTagContainer* ExtraTags = nullptr
	) const;

	// Convenience wrappers (use canonical tags)
	void EmitPrimitiveStarted(const FSpellExecContextV3& Ctx) const;
	void EmitPrimitiveStopped(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason) const;
	void EmitPrimitiveTick(const FSpellExecContextV3& Ctx, float DeltaSeconds) const;
	void EmitPrimitiveHit(const FSpellExecContextV3& Ctx, float Magnitude = 1.f, const FGameplayTagContainer* ExtraTags = nullptr) const;
	void EmitPrimitiveEnter(const FSpellExecContextV3& Ctx, const FGameplayTagContainer* ExtraTags = nullptr) const;
	void EmitPrimitiveStay(const FSpellExecContextV3& Ctx, const FGameplayTagContainer* ExtraTags = nullptr) const;
	void EmitPrimitiveExit(const FSpellExecContextV3& Ctx, const FGameplayTagContainer* ExtraTags = nullptr) const;
};

// ========================= Inline impl (header-only on purpose)
// =========================

inline USpellEventBusSubsystemV3* UDeliveryDriverBaseV3::GetBus(const FSpellExecContextV3& Ctx) const
{
	// Prefer the explicit bus from context (deterministic routing), fallback to world subsystem.
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

inline void UDeliveryDriverBaseV3::EmitPrimitiveEvent(
	const FSpellExecContextV3& Ctx,
	const FGameplayTag& EventTag,
	float Magnitude,
	const FGameplayTagContainer* ExtraTags
) const
{
	USpellEventBusSubsystemV3* Bus = GetBus(Ctx);
	if (!Bus || !EventTag.IsValid())
	{
		return;
	}

	FSpellEventV3 Ev;
	Ev.RuntimeGuid = Ctx.RuntimeGuid;   // expected field in your final context
	Ev.EventTag = EventTag;
	Ev.Instigator = Ctx.Caster;
	Ev.Magnitude = Magnitude;

	// Minimal payload (Phase 4): tags only. Later phases can attach DeliveryEventContext.
	if (ExtraTags)
	{
		Ev.Tags.AppendTags(*ExtraTags);
	}

	Bus->Emit(Ev);
}

inline void UDeliveryDriverBaseV3::EmitPrimitiveStarted(const FSpellExecContextV3& Ctx) const
{
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
	EmitPrimitiveEvent(Ctx, Tags.Event_Delivery_Primitive_Started, /*Magnitude*/0.f, nullptr);
}

inline void UDeliveryDriverBaseV3::EmitPrimitiveStopped(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason) const
{
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
	EmitPrimitiveEvent(Ctx, Tags.Event_Delivery_Primitive_Stopped, /*Magnitude*/(float)(int32)Reason, nullptr);
}

inline void UDeliveryDriverBaseV3::EmitPrimitiveTick(const FSpellExecContextV3& Ctx, float DeltaSeconds) const
{
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
	EmitPrimitiveEvent(Ctx, Tags.Event_Delivery_Primitive_Tick, /*Magnitude*/DeltaSeconds, nullptr);
}

inline void UDeliveryDriverBaseV3::EmitPrimitiveHit(const FSpellExecContextV3& Ctx, float Magnitude, const FGameplayTagContainer* ExtraTags) const
{
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
	EmitPrimitiveEvent(Ctx, Tags.Event_Delivery_Primitive_Hit, Magnitude, ExtraTags);
}

inline void UDeliveryDriverBaseV3::EmitPrimitiveEnter(const FSpellExecContextV3& Ctx, const FGameplayTagContainer* ExtraTags) const
{
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
	EmitPrimitiveEvent(Ctx, Tags.Event_Delivery_Primitive_Enter, 0.f, ExtraTags);
}

inline void UDeliveryDriverBaseV3::EmitPrimitiveStay(const FSpellExecContextV3& Ctx, const FGameplayTagContainer* ExtraTags) const
{
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
	EmitPrimitiveEvent(Ctx, Tags.Event_Delivery_Primitive_Stay, 0.f, ExtraTags);
}

inline void UDeliveryDriverBaseV3::EmitPrimitiveExit(const FSpellExecContextV3& Ctx, const FGameplayTagContainer* ExtraTags) const
{
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
	EmitPrimitiveEvent(Ctx, Tags.Event_Delivery_Primitive_Exit, 0.f, ExtraTags);
}
