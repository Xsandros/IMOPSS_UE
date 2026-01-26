#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"

#include "Delivery/Rig/DeliveryRigNodeV3.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

static FTransform GetAttachBaseWS(UWorld* World, AActor* Caster, const FDeliveryAttachV3& Attach)
{
	switch (Attach.Mode)
	{
		case EDeliveryAttachModeV3::World:
		{
			// World origin with local offset
			return Attach.LocalOffset;
		}

		case EDeliveryAttachModeV3::CasterSocket:
		{
			if (Caster)
			{
				// If you later want skeletal sockets, wire it here (component lookup).
				// For now: treat as actor transform + local.
				return Attach.LocalOffset * Caster->GetActorTransform();
			}
			return Attach.LocalOffset;
		}

		case EDeliveryAttachModeV3::TargetActor:
		{
			if (AActor* Target = Attach.TargetActor.Get())
			{
				return Attach.LocalOffset * Target->GetActorTransform();
			}
			return Attach.LocalOffset;
		}

		case EDeliveryAttachModeV3::Caster:
		default:
		{
			if (Caster)
			{
				return Attach.LocalOffset * Caster->GetActorTransform();
			}
			return Attach.LocalOffset;
		}
	}
}

void UDeliveryRigEvaluatorV3::Evaluate(
	UWorld* World,
	AActor* Caster,
	const FDeliveryAttachV3& Attach,
	const UDeliveryRigV3* Rig,
	float TimeSeconds,
	FDeliveryRigEvalResultV3& OutResult)
{
	OutResult.RootWorld = FTransform::Identity;
	OutResult.EmittersWorld.Reset();

	// Base from attach
	const FTransform BaseWS = GetAttachBaseWS(World, Caster, Attach);

	// No rig => root = base, no emitters
	if (!Rig)
	{
		OutResult.RootWorld = BaseWS;
		return;
	}

	// Evaluate root pose from rig (local space), then convert to WS.
	// Your Rig class is assumed to hold Nodes that can be evaluated at time t.
	// We keep this extremely compatible: if you already have EvaluateRoot/EvaluateEmitters functions,
	// implement them inside UDeliveryRigV3 and call them here.
	FTransform RootLS = FTransform::Identity;
	TArray<FTransform> EmittersLS;

	// --- Minimal contract (must exist in your Rig) ---
	// If your current UDeliveryRigV3 already has different APIs, change ONLY these calls
	// to match your existing implementation (no other code depends on internal rig structure).
	Rig->Evaluate(TimeSeconds, RootLS, EmittersLS);
	// -----------------------------------------------

	OutResult.RootWorld = RootLS * BaseWS;

	OutResult.EmittersWorld.Reserve(EmittersLS.Num());
	for (const FTransform& ELS : EmittersLS)
	{
		OutResult.EmittersWorld.Add(ELS * OutResult.RootWorld);
	}
}
