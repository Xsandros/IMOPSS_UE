#include "Delivery/Blackboard/DeliveryBlackboardV3.h"

static bool EnsureMsg(bool bCond, const TCHAR* Msg)
{
	ensureMsgf(bCond, TEXT("%s"), Msg);
	return bCond;
}

void FDeliveryBlackboardV3::InitFromSpec(const FDeliveryBlackboardInitSpecV3& InitSpec, const FDeliveryOwnershipRulesV3& InRules)
{
	Rules = InRules;
	Keys.Reset();

	for (const FDeliveryBBKeySpecV3& K : InitSpec.Keys)
	{
		if (K.Key == NAME_None)
		{
			continue;
		}

		FDeliveryBBRegisteredKeyV3 R;
		R.Spec = K;
		R.Value.Type = K.Type;
		Keys.Add(K.Key, R);
	}

	// Apply initial values (only if key exists + type matches)
	for (const FDeliveryBBInitValueV3& V : InitSpec.InitialValues)
	{
		if (V.Key == NAME_None)
		{
			continue;
		}

		auto* Found = Keys.Find(V.Key);
		if (!Found)
		{
			continue;
		}

		FDeliveryBBValueV3 NV;
		NV.Type = V.Type;

		switch (V.Type)
		{
			case EDeliveryBBValueTypeV3::Bool:      NV.BoolValue = V.BoolValue; break;
			case EDeliveryBBValueTypeV3::Int:       NV.IntValue = V.IntValue; break;
			case EDeliveryBBValueTypeV3::Float:     NV.FloatValue = V.FloatValue; break;
			case EDeliveryBBValueTypeV3::Vector:    NV.VectorValue = V.VectorValue; break;
			case EDeliveryBBValueTypeV3::Rotator:   NV.RotatorValue = V.RotatorValue; break;
			case EDeliveryBBValueTypeV3::Transform: NV.TransformValue = V.TransformValue; break;
			default: break;
		}

		// Initial values are treated as Group writes (safe default)
		CurrentPhase = EDeliveryBBPhaseV3::Time;
		SetValue(V.Key, NV);
	}
}

void FDeliveryBlackboardV3::BeginPhase(EDeliveryBBPhaseV3 Phase)
{
	CurrentPhase = Phase;
}

bool FDeliveryBlackboardV3::CanWrite(FName Key, EDeliveryBBValueTypeV3 ExpectedType, EDeliveryBBOwnerV3 Writer) const
{
	const FDeliveryBBRegisteredKeyV3* R = Keys.Find(Key);
	if (!R)
	{
		if (Rules.bStrict) { EnsureMsg(false, TEXT("DeliveryBlackboard: Write to unregistered key.")); }
		return false;
	}

	if (R->Spec.Type != ExpectedType)
	{
		if (Rules.bStrict) { EnsureMsg(false, TEXT("DeliveryBlackboard: Write type mismatch.")); }
		return false;
	}

	if (R->Spec.Owner != Writer)
	{
		if (Rules.bStrict) { EnsureMsg(false, TEXT("DeliveryBlackboard: Write owner mismatch.")); }
		return false;
	}

	const uint8 Mask = R->Spec.WritePhaseMask;
	const uint8 Bit = uint8(1u << uint8(CurrentPhase));
	if ((Mask & Bit) == 0)
	{
		if (Rules.bStrict) { EnsureMsg(false, TEXT("DeliveryBlackboard: Write in forbidden phase.")); }
		return false;
	}

	return true;
}

bool FDeliveryBlackboardV3::SetValue(FName Key, const FDeliveryBBValueV3& NewValue)
{
	auto* R = Keys.Find(Key);
	if (!R)
	{
		return false;
	}

	R->Value = NewValue;
	return true;
}

// Reads
bool FDeliveryBlackboardV3::ReadBool(FName Key, bool DefaultValue) const
{
	const auto* R = Keys.Find(Key);
	return (R && R->Value.Type == EDeliveryBBValueTypeV3::Bool) ? R->Value.BoolValue : DefaultValue;
}

int32 FDeliveryBlackboardV3::ReadInt(FName Key, int32 DefaultValue) const
{
	const auto* R = Keys.Find(Key);
	return (R && R->Value.Type == EDeliveryBBValueTypeV3::Int) ? R->Value.IntValue : DefaultValue;
}

float FDeliveryBlackboardV3::ReadFloat(FName Key, float DefaultValue) const
{
	const auto* R = Keys.Find(Key);
	return (R && R->Value.Type == EDeliveryBBValueTypeV3::Float) ? R->Value.FloatValue : DefaultValue;
}

FVector FDeliveryBlackboardV3::ReadVector(FName Key, const FVector& DefaultValue) const
{
	const auto* R = Keys.Find(Key);
	return (R && R->Value.Type == EDeliveryBBValueTypeV3::Vector) ? R->Value.VectorValue : DefaultValue;
}

FRotator FDeliveryBlackboardV3::ReadRotator(FName Key, const FRotator& DefaultValue) const
{
	const auto* R = Keys.Find(Key);
	return (R && R->Value.Type == EDeliveryBBValueTypeV3::Rotator) ? R->Value.RotatorValue : DefaultValue;
}

FTransform FDeliveryBlackboardV3::ReadTransform(FName Key, const FTransform& DefaultValue) const
{
	const auto* R = Keys.Find(Key);
	return (R && R->Value.Type == EDeliveryBBValueTypeV3::Transform) ? R->Value.TransformValue : DefaultValue;
}

// Writes
bool FDeliveryBlackboardV3::WriteBool(FName Key, bool V, EDeliveryBBOwnerV3 Writer)
{
	if (!CanWrite(Key, EDeliveryBBValueTypeV3::Bool, Writer))
	{
		return !Rules.bIgnoreInvalidWrites;
	}
	FDeliveryBBValueV3 NV; NV.Type = EDeliveryBBValueTypeV3::Bool; NV.BoolValue = V;
	return SetValue(Key, NV);
}

bool FDeliveryBlackboardV3::WriteInt(FName Key, int32 V, EDeliveryBBOwnerV3 Writer)
{
	if (!CanWrite(Key, EDeliveryBBValueTypeV3::Int, Writer))
	{
		return !Rules.bIgnoreInvalidWrites;
	}
	FDeliveryBBValueV3 NV; NV.Type = EDeliveryBBValueTypeV3::Int; NV.IntValue = V;
	return SetValue(Key, NV);
}

bool FDeliveryBlackboardV3::WriteFloat(FName Key, float V, EDeliveryBBOwnerV3 Writer)
{
	if (!CanWrite(Key, EDeliveryBBValueTypeV3::Float, Writer))
	{
		return !Rules.bIgnoreInvalidWrites;
	}
	FDeliveryBBValueV3 NV; NV.Type = EDeliveryBBValueTypeV3::Float; NV.FloatValue = V;
	return SetValue(Key, NV);
}

bool FDeliveryBlackboardV3::WriteVector(FName Key, const FVector& V, EDeliveryBBOwnerV3 Writer)
{
	if (!CanWrite(Key, EDeliveryBBValueTypeV3::Vector, Writer))
	{
		return !Rules.bIgnoreInvalidWrites;
	}
	FDeliveryBBValueV3 NV; NV.Type = EDeliveryBBValueTypeV3::Vector; NV.VectorValue = V;
	return SetValue(Key, NV);
}

bool FDeliveryBlackboardV3::WriteRotator(FName Key, const FRotator& V, EDeliveryBBOwnerV3 Writer)
{
	if (!CanWrite(Key, EDeliveryBBValueTypeV3::Rotator, Writer))
	{
		return !Rules.bIgnoreInvalidWrites;
	}
	FDeliveryBBValueV3 NV; NV.Type = EDeliveryBBValueTypeV3::Rotator; NV.RotatorValue = V;
	return SetValue(Key, NV);
}

bool FDeliveryBlackboardV3::WriteTransform(FName Key, const FTransform& V, EDeliveryBBOwnerV3 Writer)
{
	if (!CanWrite(Key, EDeliveryBBValueTypeV3::Transform, Writer))
	{
		return !Rules.bIgnoreInvalidWrites;
	}
	FDeliveryBBValueV3 NV; NV.Type = EDeliveryBBValueTypeV3::Transform; NV.TransformValue = V;
	return SetValue(Key, NV);
}
