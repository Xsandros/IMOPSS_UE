#include "Effects/EffectResolverSubsystemV3.h"
#include "Effects/EffectResolverV3.h"

void UEffectResolverSubsystemV3::SetMitigationProvider(TScriptInterface<IEffectMitigationProviderV3> InProvider)
{
	Provider = InProvider;
}

bool UEffectResolverSubsystemV3::ResolveAndApplyToTargets(
	const FSpellExecContextV3& ExecCtx,
	const FEffectSpecV3& Spec,
	const TArray<TWeakObjectPtr<AActor>>& Targets,
	TArray<FEffectResultV3>& OutResults) const
{
	OutResults.Reset();
	OutResults.Reserve(Targets.Num());

	bool bAnyApplied = false;

	for (const TWeakObjectPtr<AActor>& T : Targets)
	{
		AActor* Target = T.Get();
		FEffectResultV3 R;
		const bool bOk = FEffectResolverV3::ResolveAndApply(ExecCtx, Spec, Target, R);
		OutResults.Add(R);
		bAnyApplied |= bOk;
	}

	return bAnyApplied;
}
