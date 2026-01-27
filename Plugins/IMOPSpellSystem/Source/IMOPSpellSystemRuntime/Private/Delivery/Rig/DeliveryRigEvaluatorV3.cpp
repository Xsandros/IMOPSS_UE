#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"

#include "Delivery/Rig/DeliveryRigV3.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

static FTransform GetAttachBaseWS(UWorld* World, AActor* Caster, const FDeliveryAttachV3& Attach)
{
	switch (Attach.Mode)
	{
	case EDeliveryAttachModeV3::World:
	{
		// World origin with local offset already applied
		return Attach.LocalOffset;
	}

	case EDeliveryAttachModeV3::CasterSocket:
	{
		if (Caster)
		{
			if (Attach.SocketName != NAME_None)
			{
				if (USceneComponent* Root = Caster->GetRootComponent())
				{
					if (Root->DoesSocketExist(Attach.SocketName))
					{
						const FTransform SocketWS = Root->GetSocketTransform(Attach.SocketName, RTS_World);
						return Attach.LocalOffset * SocketWS;
					}
				}
			}

			// Fallback: caster transform
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
	OutResult.EmitterNames.Reset();
	OutResult.AnchorsWorld.Reset();

	const FTransform BaseWS = GetAttachBaseWS(World, Caster, Attach);

	// No rig => root = base, no emitters
	if (!Rig)
	{
		OutResult.RootWorld = BaseWS;
		return;
	}

	// Evaluate local-space from rig
	FTransform RootLS = FTransform::Identity;
	TArray<FTransform> EmittersLS;
	Rig->Evaluate(TimeSeconds, RootLS, EmittersLS);

	OutResult.RootWorld = RootLS * BaseWS;

	OutResult.EmittersWorld.Reserve(EmittersLS.Num());
	for (const FTransform& ELS : EmittersLS)
	{
		OutResult.EmittersWorld.Add(ELS * OutResult.RootWorld);
	}

	// Forward-compatible: propagate emitter names if authoring provided them
	if (Rig->EmitterNames.Num() == OutResult.EmittersWorld.Num())
	{
		OutResult.EmitterNames = Rig->EmitterNames;
	}
}
