#include "Effects/Exec_ApplyForceV3.h"

#include "Effects/SpellPayloadsEffectsV3.h"
#include "Effects/EffectResolverSubsystemV3.h"
#include "Stores/SpellTargetStoreV3.h"

static UWorld* GetWorldFromExecCtx(const FSpellExecContextV3& Ctx)
{
	return Ctx.WorldContext ? Ctx.WorldContext->GetWorld() : nullptr;
}

static UGameInstance* GetGIFromExecCtx(const FSpellExecContextV3& Ctx)
{
	if (UWorld* W = GetWorldFromExecCtx(Ctx))
	{
		return W->GetGameInstance();
	}
	return nullptr;
}


const UScriptStruct* UExec_ApplyForceV3::GetPayloadStruct() const
{
	return FPayload_EffectApplyForceV3::StaticStruct();
}

void UExec_ApplyForceV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	const FPayload_EffectApplyForceV3* P = static_cast<const FPayload_EffectApplyForceV3*>(PayloadData);
	if (!P || !Ctx.TargetStore || !Ctx.WorldContext) return;

	if (!Ctx.bAuthority) return;

	FTargetSetV3 Set;
	if (!Ctx.TargetStore->Get(P->TargetSet, Set)) return;

	TArray<TWeakObjectPtr<AActor>> Targets;
	Targets.Reserve(Set.Targets.Num());
	for (const FTargetRefV3& R : Set.Targets) Targets.Add(R.Actor);

	if (UGameInstance* GI = GetGIFromExecCtx(Ctx))
	{
		if (!GI) return;
		if (UEffectResolverSubsystemV3* Res = GI->GetSubsystem<UEffectResolverSubsystemV3>())
		{
			TArray<FEffectResultV3> Results;
			Res->ResolveAndApplyToTargets(Ctx, P->Effect, Targets, Results);

			// TODO "final": emit Event.Effect.* via EventBus
		}
	}
}
