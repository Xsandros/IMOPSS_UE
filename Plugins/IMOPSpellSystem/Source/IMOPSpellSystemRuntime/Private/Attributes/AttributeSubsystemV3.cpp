#include "Attributes/AttributeSubsystemV3.h"

void UAttributeSubsystemV3::SetProvider(TScriptInterface<IAttributeProviderV3> InProvider)
{
	Provider = InProvider;
}

bool UAttributeSubsystemV3::GetAttribute(AActor* Target, const FAttributeKeyV3& Key, float& OutValue) const
{
	if (!Provider.GetObject() || !Target)
	{
		OutValue = 0.f;
		return false;
	}
	return Provider->GetAttribute(Target, Key, OutValue);
}

bool UAttributeSubsystemV3::ApplyOp(AActor* Target, const FAttributeKeyV3& Key, EAttributeOpV3 Op, float Operand, float& OutNewValue) const
{
	if (!Provider.GetObject() || !Target)
	{
		OutNewValue = 0.f;
		return false;
	}
	return Provider->ApplyOp(Target, Key, Op, Operand, OutNewValue);
}
