#include "Effects/EffectResistanceComponentV3.h"

float UEffectResistanceComponentV3::GetResistanceForTag(FGameplayTag ResistanceTag) const
{
	if (!ResistanceTag.IsValid()) return 0.f;
	if (const float* V = Resistances.Find(ResistanceTag))
	{
		return FMath::Clamp(*V, 0.f, 1.f);
	}
	return 0.f;
}
