#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliveryContextV3.h"

#include "Events/SpellEventV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "Runtime/SpellRuntimeV3.h"

#include "DeliveryDriverBaseV3.generated.h"

struct FSpellExecContextV3;
class UDeliveryGroupRuntimeV3;

/**
 * Base class for all Delivery drivers (Composite-first).
 *
 * Lifecycle:
 *  - Start(Ctx, Group, PrimitiveCtx)
 *  - Tick(Ctx, Group, DeltaSeconds) (optional)
 *  - Stop(Ctx, Group, Reason)
 *
 * Notes:
 *  - Drivers are owned by DeliverySubsystemV3 / GroupRuntime and are server-authoritative.
 *  - Drivers keep minimal state + can read/write Group->Blackboard and Group->PrimitiveCtxById.
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
	virtual void Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) PURE_VIRTUAL(UDeliveryDriverBaseV3::Start, );
	virtual void Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds) {}
	virtual void Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason) PURE_VIRTUAL(UDeliveryDriverBaseV3::Stop, );

	bool IsActive() const { return bActive; }

protected:
	// ===== Event helpers (lightweight envelope only) =====

	static const FGuid& ResolveRuntimeGuid(const FSpellExecContextV3& Ctx)
	{
		if (const USpellRuntimeV3* R = Cast<USpellRuntimeV3>(Ctx.Runtime.Get()))
		{
			return R->GetRuntimeGuid();
		}
		static FGuid Dummy;
		return Dummy;
	}

	static USpellEventBusSubsystemV3* ResolveEventBus(const FSpellExecContextV3& Ctx)
	{
		if (USpellEventBusSubsystemV3* Bus = Cast<USpellEventBusSubsystemV3>(Ctx.EventBus.Get()))
		{
			return Bus;
		}
		if (UWorld* W = Ctx.GetWorld())
		{
			return W->GetSubsystem<USpellEventBusSubsystemV3>();
		}
		return nullptr;
	}

	void EmitPrimitiveEvent(const FSpellExecContextV3& Ctx, const FGameplayTag& Tag, float Magnitude = 0.f, const FGameplayTagContainer& ExtraTags = FGameplayTagContainer()) const
	{
		USpellEventBusSubsystemV3* Bus = ResolveEventBus(Ctx);
		if (!Bus)
		{
			return;
		}

		FSpellEventV3 Ev;
		Ev.RuntimeGuid = ResolveRuntimeGuid(Ctx);
		Ev.EventTag = Tag;
		Ev.Caster = Ctx.GetCaster();

		Ev.Magnitude = Magnitude;

		// Always include identity tags (very useful for debugging, stop policies later)
		Ev.Tags = ExtraTags;
		Ev.Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Spell.Delivery")));
		Ev.Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Spell.Delivery.DeliveryId")));
		// Encode names in tags is not ideal; we keep it minimal: rely on Trace/Blackboard later.
		// For now, add "semantic" tags if they exist in your tag list:
		// (No hard dependency here.)

		Bus->Emit(Ev);
	}

	void EmitPrimitiveStarted(const FSpellExecContextV3& Ctx) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Started);
	}

	void EmitPrimitiveStopped(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 /*Reason*/) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Stopped);
	}

	void EmitPrimitiveTick(const FSpellExecContextV3& Ctx, float DeltaSeconds) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Tick, DeltaSeconds);
	}

	void EmitPrimitiveHit(const FSpellExecContextV3& Ctx, float Magnitude = 1.f, UObject* /*OptionalPayload*/ = nullptr) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Hit, Magnitude);
	}

	void EmitPrimitiveEnter(const FSpellExecContextV3& Ctx) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Enter);
	}

	void EmitPrimitiveStay(const FSpellExecContextV3& Ctx) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Stay);
	}

	void EmitPrimitiveExit(const FSpellExecContextV3& Ctx) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Exit);
	}
};
