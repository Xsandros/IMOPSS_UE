#include "Effects/EffectImmunityComponentV3.h"

bool UEffectImmunityComponentV3::HasImmunityTag(FGameplayTag ImmunityTag) const
{
	return ImmunityTag.IsValid() && Immunities.HasTag(ImmunityTag);
}
