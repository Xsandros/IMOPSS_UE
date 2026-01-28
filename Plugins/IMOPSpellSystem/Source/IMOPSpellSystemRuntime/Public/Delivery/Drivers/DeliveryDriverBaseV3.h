#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliveryContextV3.h"
#include "DrawDebugHelpers.h"

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

	void EmitPrimitiveEvent(
		const FSpellExecContextV3& Ctx,
		const FGameplayTag& Tag,
		float Magnitude,
		const FGameplayTagContainer& ExtraTags
	) const
	{
		USpellEventBusSubsystemV3* Bus = ResolveEventBus(Ctx);
		if (!Bus) { return; }

		FSpellEventV3 Ev;
		Ev.RuntimeGuid = ResolveRuntimeGuid(Ctx);
		Ev.EventTag    = Tag;
		Ev.Caster      = Ctx.GetCaster();
		Ev.Magnitude   = Magnitude;

		// Only add tags that are guaranteed to exist:
		Ev.Tags = ExtraTags;

		// IMPORTANT: Avoid RequestGameplayTag() here (can ensure if not registered).
		// If you have explicit fields for identity in FSpellEventV3, set them here.
		// Example (only if those fields exist in your struct):
		// Ev.DeliveryHandle = GroupHandle;
		// Ev.DeliveryPrimitiveId = PrimitiveId;

		Bus->Emit(Ev);
	}

	void EmitPrimitiveStarted(const FSpellExecContextV3& Ctx, const FGameplayTagContainer& ExtraTags) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Started, 0.f, ExtraTags);
	}

	void EmitPrimitiveStopped(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 /*Reason*/, const FGameplayTagContainer& ExtraTags) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Stopped, 0.f, ExtraTags);
	}

	void EmitPrimitiveTick(const FSpellExecContextV3& Ctx, float DeltaSeconds, const FGameplayTagContainer& ExtraTags) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Tick, DeltaSeconds, ExtraTags);
	}

	void EmitPrimitiveHit(const FSpellExecContextV3& Ctx, float Magnitude, UObject* /*OptionalPayload*/, const FGameplayTagContainer& ExtraTags) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Hit, Magnitude, ExtraTags);
	}

	void EmitPrimitiveEnter(const FSpellExecContextV3& Ctx, const FGameplayTagContainer& ExtraTags) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Enter, 0.f, ExtraTags);
	}

	void EmitPrimitiveStay(const FSpellExecContextV3& Ctx, const FGameplayTagContainer& ExtraTags) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Stay, 0.f, ExtraTags);
	}

	void EmitPrimitiveExit(const FSpellExecContextV3& Ctx, const FGameplayTagContainer& ExtraTags) const
	{
		EmitPrimitiveEvent(Ctx, FIMOPSpellGameplayTagsV3::Get().Event_Delivery_Primitive_Exit, 0.f, ExtraTags);
	}

	
	protected:
    bool IsDebugDrawEnabled() const
    {
        // declare as extern if needed, or duplicate a CVar in base
        static const auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("imop.Delivery.DebugDraw"));
        return CVar && CVar->GetValueOnGameThread() > 0;
    }

    float GetDebugDrawLife() const
    {
        static const auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("imop.Delivery.DebugDrawLife"));
        return CVar ? CVar->GetValueOnGameThread() : 0.05f;
    }

	void DebugDrawPrimitiveShape(
		UWorld* World,
		const FDeliveryPrimitiveSpecV3& Spec,
		const FTransform& PoseWS,
		const FDeliveryDebugDrawConfigV3& DebugCfg
	) const
    {
    	if (!World || !ShouldOverlayDraw(DebugCfg))
    	{
    		return;
    	}

    	const float Life = (IsOverlayCVarEnabled() ? GetOverlayLife() : DebugCfg.Duration);

    	const FString Label = FString::Printf(TEXT("%s | %s"),
			*Spec.PrimitiveId.ToString(),
			*UEnum::GetValueAsString(Spec.Kind));

    	DrawDebugString(World, PoseWS.GetLocation(), Label, nullptr, FColor::White, Life, false);

    	switch (Spec.Shape.Kind)
    	{
    	case EDeliveryShapeV3::Sphere:
    		DrawDebugSphere(World, PoseWS.GetLocation(), Spec.Shape.Radius, 12, FColor::White, false, Life);
    		break;
    	case EDeliveryShapeV3::Box:
    		DrawDebugBox(World, PoseWS.GetLocation(), Spec.Shape.Extents, PoseWS.GetRotation(), FColor::White, false, Life);
    		break;
    	case EDeliveryShapeV3::Capsule:
    		DrawDebugCapsule(World, PoseWS.GetLocation(), Spec.Shape.HalfHeight, Spec.Shape.CapsuleRadius, PoseWS.GetRotation(), FColor::White, false, Life);
    		break;
    	case EDeliveryShapeV3::Ray:
    		{
    			const FVector Start = PoseWS.GetLocation();
    			const FVector End   = Start + PoseWS.GetRotation().GetForwardVector() * Spec.Shape.RayLength;
    			DrawDebugLine(World, Start, End, FColor::White, false, Life, 0, 1.5f);
    			break;
    		}
    	default:
    		break;
    	}
    }


	void DebugDrawBeamLine(
	  UWorld* World,
	  const FDeliveryPrimitiveSpecV3& Spec,
	  const FVector& StartWS,
	  const FVector& EndWS,
	  const FDeliveryDebugDrawConfigV3& DebugCfg
  ) const
    {
    	if (!World || !ShouldOverlayDraw(DebugCfg))
    	{
    		return;
    	}

    	const float Life = (IsOverlayCVarEnabled() ? GetOverlayLife() : DebugCfg.Duration);

    	const FString Label = FString::Printf(TEXT("%s | %s"),
			*Spec.PrimitiveId.ToString(),
			*UEnum::GetValueAsString(Spec.Kind));

    	DrawDebugLine(World, StartWS, EndWS, FColor::White, false, Life, 0, 1.5f);
    	DrawDebugString(World, StartWS, Label, nullptr, FColor::White, Life, false);
    }

protected:
	bool IsOverlayCVarEnabled() const
	{
		static const auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("imop.Delivery.DebugDraw"));
		return CVar && CVar->GetValueOnGameThread() > 0;
	}

	float GetOverlayLife() const
	{
		static const auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("imop.Delivery.DebugDrawLife"));
		return CVar ? CVar->GetValueOnGameThread() : 0.05f;
	}
	
	bool ShouldOverlayDraw(const FDeliveryDebugDrawConfigV3& DebugCfg) const
	{
		return IsOverlayCVarEnabled() || DebugCfg.bEnable;
	}



};
