#include "Attributes/SpellAttributeComponentV3.h"

USpellAttributeComponentV3::USpellAttributeComponentV3()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool USpellAttributeComponentV3::GetAttribute(FGameplayTag AttributeTag, float& OutValue) const
{
	if (!AttributeTag.IsValid())
	{
		OutValue = 0.f;
		return false;
	}

	if (const float* Found = Attributes.Find(AttributeTag))
	{
		OutValue = *Found;
		return true;
	}

	OutValue = 0.f;
	return false;
}

void USpellAttributeComponentV3::SetAttribute(FGameplayTag AttributeTag, float NewValue)
{
	if (!AttributeTag.IsValid()) return;
	Attributes.Add(AttributeTag, NewValue);
}

void USpellAttributeComponentV3::AddDelta(FGameplayTag AttributeTag, float Delta, float& OutNewValue)
{
	float Cur = 0.f;
	GetAttribute(AttributeTag, Cur);
	OutNewValue = Cur + Delta;
	SetAttribute(AttributeTag, OutNewValue);
}

void USpellAttributeComponentV3::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpellAttributeComponentV3, Attributes);
}
