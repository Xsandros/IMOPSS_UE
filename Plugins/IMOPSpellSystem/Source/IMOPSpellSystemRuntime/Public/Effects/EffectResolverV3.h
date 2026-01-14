#pragma once

#include "CoreMinimal.h"
#include "Effects/EffectSpecV3.h"
#include "Effects/EffectResultV3.h"
#include "Runtime/SpellRuntimeV3.h"
#include "Actions/SpellPayloadsCoreV3.h"


struct IMOPSPELLSYSTEMRUNTIME_API FEffectResolverV3
{
	static bool ResolveAndApply(
		const FSpellExecContextV3& ExecCtx,
		const FEffectSpecV3& Spec,
		AActor* Target,
		FEffectResultV3& OutResult);

private:
	static bool TryGetNumericFromSpellValue(const FSpellValueV3& V, float& Out);

	static float EvaluateMagnitude(
		const FSpellExecContextV3& ExecCtx,
		const FMagnitudeV3& Magnitude,
		AActor* Target,
		float& OutRaw);

	static bool ApplyModifyAttribute(
		const FSpellExecContextV3& ExecCtx,
		const FEffectSpecV3& Spec,
		AActor* Target,
		float FinalMagnitude,
		FEffectResultV3& OutResult);

	static bool ApplyForce(
		const FSpellExecContextV3& ExecCtx,
		const FEffectSpecV3& Spec,
		AActor* Target,
		FEffectResultV3& OutResult);
};
