#include "Attributes/DefaultAttributeProviderV3.h"
#include "Attributes/SpellAttributeComponentV3.h"

static USpellAttributeComponentV3* FindAttrComp(AActor* Target)
{
	return Target ? Target->FindComponentByClass<USpellAttributeComponentV3>() : nullptr;
}

bool UDefaultAttributeProviderV3::GetAttribute(AActor* Target, const FAttributeKeyV3& Key, float& OutValue) const
{
	if (USpellAttributeComponentV3* C = FindAttrComp(Target))
	{
		return C->GetAttribute(Key.AttributeTag, OutValue);
	}
	OutValue = 0.f;
	return false;
}

bool UDefaultAttributeProviderV3::SetAttribute(AActor* Target, const FAttributeKeyV3& Key, float NewValue) const
{
	if (USpellAttributeComponentV3* C = FindAttrComp(Target))
	{
		C->SetAttribute(Key.AttributeTag, NewValue);
		return true;
	}
	return false;
}

bool UDefaultAttributeProviderV3::ApplyOp(AActor* Target, const FAttributeKeyV3& Key, EAttributeOpV3 Op, float Operand, float& OutNewValue) const
{
	float Cur = 0.f;
	if (!GetAttribute(Target, Key, Cur))
	{
		// If missing, treat as 0 and still allow ops (useful for health init etc.)
		Cur = 0.f;
	}

	switch (Op)
	{
	case EAttributeOpV3::Add:      OutNewValue = Cur + Operand; break;
	case EAttributeOpV3::Sub:      OutNewValue = Cur - Operand; break;
	case EAttributeOpV3::Mul:      OutNewValue = Cur * Operand; break;
	case EAttributeOpV3::Div:      OutNewValue = (FMath::IsNearlyZero(Operand) ? Cur : (Cur / Operand)); break;
	case EAttributeOpV3::Override: OutNewValue = Operand; break;
	case EAttributeOpV3::Min:      OutNewValue = FMath::Min(Cur, Operand); break;
	case EAttributeOpV3::Max:      OutNewValue = FMath::Max(Cur, Operand); break;
	default:                       OutNewValue = Cur; break;
	}

	return SetAttribute(Target, Key, OutNewValue);
}
