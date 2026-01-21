#include "Effects/Exec_ModifyAttributeV3.h"
#include "Core/SpellExecContextHelpersV3.h"

#include "Effects/SpellPayloadsEffectsV3.h"
#include "Effects/EffectResolverSubsystemV3.h"
#include "Stores/SpellTargetStoreV3.h"


const UScriptStruct* UExec_ModifyAttributeV3::GetPayloadStruct() const
{
	return FPayload_EffectModifyAttributeV3::StaticStruct();
}

void UExec_ModifyAttributeV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	const FPayload_EffectModifyAttributeV3* P = static_cast<const FPayload_EffectModifyAttributeV3*>(PayloadData);

	if (!P)
	{
		UE_LOG(LogTemp, Warning, TEXT("Exec_ModifyAttribute: payload null"));
		return;
	}
	if (!Ctx.WorldContext)
	{
		UE_LOG(LogTemp, Warning, TEXT("Exec_ModifyAttribute: WorldContext missing"));
		return;
	}
	if (!Ctx.TargetStore)
	{
		UE_LOG(LogTemp, Warning, TEXT("Exec_ModifyAttribute: TargetStore missing"));
		return;
	}
	if (!Ctx.bAuthority)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Exec_ModifyAttribute: not authority -> skip (TargetSet=%s)"), *P->TargetSet.ToString());
		return;
	}

	FTargetSetV3 Set;
	if (!Ctx.TargetStore->Get(P->TargetSet, Set))
	{
		UE_LOG(LogTemp, Warning, TEXT("Exec_ModifyAttribute: TargetSet not found: %s"), *P->TargetSet.ToString());
		return;
	}

	if (Set.Targets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Exec_ModifyAttribute: TargetSet empty: %s"), *P->TargetSet.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Exec_ModifyAttribute: apply Effect to TargetSet=%s Count=%d"),
		*P->TargetSet.ToString(), Set.Targets.Num());

	TArray<TWeakObjectPtr<AActor>> Targets;
	Targets.Reserve(Set.Targets.Num());
	for (const FTargetRefV3& R : Set.Targets)
	{
		Targets.Add(R.Actor);
	}


	if (UGameInstance* GI = IMOP_GetGIFromExecCtx(Ctx))
	{
		if (!GI) return;
		if (UEffectResolverSubsystemV3* Res = GI->GetSubsystem<UEffectResolverSubsystemV3>())
		{
			TArray<FEffectResultV3> Results;
			Res->ResolveAndApplyToTargets(Ctx, P->Effect, Targets, Results);
			
			UE_LOG(LogTemp, Log, TEXT("Exec_ModifyAttribute: done (targets=%d results=%d)"),
	Targets.Num(), Results.Num());


			// TODO "final": emit Event.Effect.* + Event.Damage/Heal derived from tags.
			// Use Ctx.EventBus->Emit(...).
		}
	}
}
