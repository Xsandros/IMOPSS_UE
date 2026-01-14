#include "Attributes/SpellAttributeComponentV3.h"

USpellAttributeComponentV3::USpellAttributeComponentV3()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool USpellAttributeComponentV3::GetAttribute(FGameplayTag AttributeTag, float& OutValue) const
{
	return ReplicatedAttributes.FindValue(AttributeTag, OutValue);
}

void USpellAttributeComponentV3::SetAttribute(FGameplayTag AttributeTag, float NewValue)
{
	// Authority should own writes; but leaving this as a pure setter is fine.
	ReplicatedAttributes.SetValue(AttributeTag, NewValue);
}

void USpellAttributeComponentV3::AddDelta(FGameplayTag AttributeTag, float Delta, float& OutNewValue)
{
	float Cur = 0.f;
	ReplicatedAttributes.FindValue(AttributeTag, Cur);
	OutNewValue = Cur + Delta;
	ReplicatedAttributes.SetValue(AttributeTag, OutNewValue);
}

void USpellAttributeComponentV3::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpellAttributeComponentV3, ReplicatedAttributes);
}
