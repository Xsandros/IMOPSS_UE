#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Effects/EffectMitigationProviderV3.h"
#include "Effects/EffectSpecV3.h"
#include "Effects/EffectResultV3.h"
#include "EffectResolverSubsystemV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UEffectResolverSubsystemV3 : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void SetMitigationProvider(TScriptInterface<IEffectMitigationProviderV3> InProvider);

	bool ResolveAndApplyToTargets(
		const FSpellExecContextV3& ExecCtx,
		const FEffectSpecV3& Spec,
		const TArray<TWeakObjectPtr<AActor>>& Targets,
		TArray<FEffectResultV3>& OutResults) const;

	const TScriptInterface<IEffectMitigationProviderV3>& GetMitigationProvider() const { return Provider; }

private:
	UPROPERTY()
	TScriptInterface<IEffectMitigationProviderV3> Provider;
};
