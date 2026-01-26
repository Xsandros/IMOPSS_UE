#include "Delivery/Blackboard/DeliveryBlackboardV3.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryBlackboardV3, Log, All);

static const TCHAR* PhaseToString(EDeliveryBBPhaseV3 P)
{
	switch (P)
	{
		case EDeliveryBBPhaseV3::Time:           return TEXT("Time");
		case EDeliveryBBPhaseV3::Rig:            return TEXT("Rig");
		case EDeliveryBBPhaseV3::DerivedMotion:  return TEXT("DerivedMotion");
		case EDeliveryBBPhaseV3::PrimitiveMotion:return TEXT("PrimitiveMotion");
		case EDeliveryBBPhaseV3::Query:          return TEXT("Query");
		case EDeliveryBBPhaseV3::Stop:           return TEXT("Stop");
		case EDeliveryBBPhaseV3::Commit:         return TEXT("Commit");
		default:                                 return TEXT("Unknown");
	}
}

static const TCHAR* OwnerToString(EDeliveryBBOwnerV3 O)
{
	switch (O)
	{
		case EDeliveryBBOwnerV3::Group:    return TEXT("Group");
		case EDeliveryBBOwnerV3::Primitive:return TEXT("Primitive");
		case EDeliveryBBOwnerV3::System:   return TEXT("System");
		default:                           return TEXT("Unknown");
	}
}

uint8 FDeliveryBlackboardV3::PhaseBit(EDeliveryBBPhaseV3 Phase)
{
	const uint8 I = (uint8)Phase;
	return (uint8)(1u << (I & 7u));
}

void FDeliveryBlackboardV3::InitFromSpec(const FDeliveryBlackboardInitSpecV3& Init, const FDeliveryOwnershipRulesV3& InRules)
{
	Rules = InRules;
	Keys.Reset();

	for (const FDeliveryBBKeySpecV3& K : Init.Keys)
	{
		if (K.Key == NAME_None)
		{
			continue;
		}

		FDeliveryBBKeyRuntimeV3 KR;
		KR.Spec = K;
		KR.Value = FDeliveryBBValueV3();
		Keys.Add(K.Key, KR);
	}

	CurrentPhase = EDeliveryBBPhaseV3::Time;
	bInitialized = true;

	UE_LOG(LogIMOPDeliveryBlackboardV3, Log, TEXT("DeliveryBlackboard Init: keys=%d enforcePhase=%d enforceOwner=%d rejectUnknown=%d"),
		Keys.Num(),
		Rules.bEnforcePhase ? 1 : 0,
		Rules.bEnforceOwner ? 1 : 0,
		Rules.bRejectUnknownKeys ? 1 : 0);
}

void FDeliveryBlackboardV3::BeginPhase(EDeliveryBBPhaseV3 Phase)
{
	CurrentPhase = Phase;
}

bool FDeliveryBlackboardV3::HasKey(FName Key) const
{
	return Keys.Contains(Key);
}

bool FDeliveryBlackboardV3::CanWrite(const FDeliveryBBKeyRuntimeV3& KR, EDeliveryBBValueTypeV3 ExpectedType, EDeliveryBBOwnerV3 Writer) const
{
	if (!bInitialized)
	{
		return false;
	}

	if (KR.Spec.Type != ExpectedType)
	{
		return false;
	}

	if (Rules.bEnforceOwner && KR.Spec.Owner != Writer)
	{
		return false;
	}

	if (Rules.bEnforcePhase)
	{
		const uint8 Bit = PhaseBit(CurrentPhase);
		if ((KR.Spec.WritePhaseMask & Bit) == 0)
		{
			return false;
		}
	}

	return true;
}

static bool RejectUnknown(const FDeliveryOwnershipRulesV3& Rules, FName Key, const TCHAR* Op)
{
	if (Rules.bRejectUnknownKeys)
	{
		UE_LOG(LogIMOPDeliveryBlackboardV3, Warning, TEXT("BB %s rejected unknown key: %s"), Op, *Key.ToString());
		return true;
	}
	return false;
}

bool FDeliveryBlackboardV3::WriteFloat(FName Key, float V, EDeliveryBBOwnerV3 Writer)
{
	FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR)
	{
		return !RejectUnknown(Rules, Key, TEXT("WriteFloat"));
	}
	if (!CanWrite(*KR, EDeliveryBBValueTypeV3::Float, Writer))
	{
		UE_LOG(LogIMOPDeliveryBlackboardV3, Verbose, TEXT("BB WriteFloat rejected: key=%s phase=%s owner=%s"),
			*Key.ToString(), PhaseToString(CurrentPhase), OwnerToString(Writer));
		return false;
	}
	KR->Value.FloatValue = V;
	return true;
}

bool FDeliveryBlackboardV3::WriteInt(FName Key, int32 V, EDeliveryBBOwnerV3 Writer)
{
	FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR)
	{
		return !RejectUnknown(Rules, Key, TEXT("WriteInt"));
	}
	if (!CanWrite(*KR, EDeliveryBBValueTypeV3::Int, Writer))
	{
		return false;
	}
	KR->Value.IntValue = V;
	return true;
}

bool FDeliveryBlackboardV3::WriteBool(FName Key, bool V, EDeliveryBBOwnerV3 Writer)
{
	FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR)
	{
		return !RejectUnknown(Rules, Key, TEXT("WriteBool"));
	}
	if (!CanWrite(*KR, EDeliveryBBValueTypeV3::Bool, Writer))
	{
		return false;
	}
	KR->Value.BoolValue = V;
	return true;
}

bool FDeliveryBlackboardV3::WriteVector(FName Key, const FVector& V, EDeliveryBBOwnerV3 Writer)
{
	FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR)
	{
		return !RejectUnknown(Rules, Key, TEXT("WriteVector"));
	}
	if (!CanWrite(*KR, EDeliveryBBValueTypeV3::Vector, Writer))
	{
		return false;
	}
	KR->Value.VectorValue = V;
	return true;
}

bool FDeliveryBlackboardV3::WriteRotator(FName Key, const FRotator& V, EDeliveryBBOwnerV3 Writer)
{
	FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR)
	{
		return !RejectUnknown(Rules, Key, TEXT("WriteRotator"));
	}
	if (!CanWrite(*KR, EDeliveryBBValueTypeV3::Rotator, Writer))
	{
		return false;
	}
	KR->Value.RotatorValue = V;
	return true;
}

bool FDeliveryBlackboardV3::WriteTransform(FName Key, const FTransform& V, EDeliveryBBOwnerV3 Writer)
{
	FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR)
	{
		return !RejectUnknown(Rules, Key, TEXT("WriteTransform"));
	}
	if (!CanWrite(*KR, EDeliveryBBValueTypeV3::Transform, Writer))
	{
		return false;
	}
	KR->Value.TransformValue = V;
	return true;
}

bool FDeliveryBlackboardV3::WriteName(FName Key, FName V, EDeliveryBBOwnerV3 Writer)
{
	FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR)
	{
		return !RejectUnknown(Rules, Key, TEXT("WriteName"));
	}
	if (!CanWrite(*KR, EDeliveryBBValueTypeV3::Name, Writer))
	{
		return false;
	}
	KR->Value.NameValue = V;
	return true;
}

// Reads
bool FDeliveryBlackboardV3::ReadFloat(FName Key, float& Out) const
{
	const FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR || KR->Spec.Type != EDeliveryBBValueTypeV3::Float) return false;
	Out = KR->Value.FloatValue;
	return true;
}
bool FDeliveryBlackboardV3::ReadInt(FName Key, int32& Out) const
{
	const FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR || KR->Spec.Type != EDeliveryBBValueTypeV3::Int) return false;
	Out = KR->Value.IntValue;
	return true;
}
bool FDeliveryBlackboardV3::ReadBool(FName Key, bool& Out) const
{
	const FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR || KR->Spec.Type != EDeliveryBBValueTypeV3::Bool) return false;
	Out = KR->Value.BoolValue;
	return true;
}
bool FDeliveryBlackboardV3::ReadVector(FName Key, FVector& Out) const
{
	const FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR || KR->Spec.Type != EDeliveryBBValueTypeV3::Vector) return false;
	Out = KR->Value.VectorValue;
	return true;
}
bool FDeliveryBlackboardV3::ReadRotator(FName Key, FRotator& Out) const
{
	const FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR || KR->Spec.Type != EDeliveryBBValueTypeV3::Rotator) return false;
	Out = KR->Value.RotatorValue;
	return true;
}
bool FDeliveryBlackboardV3::ReadTransform(FName Key, FTransform& Out) const
{
	const FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR || KR->Spec.Type != EDeliveryBBValueTypeV3::Transform) return false;
	Out = KR->Value.TransformValue;
	return true;
}
bool FDeliveryBlackboardV3::ReadName(FName Key, FName& Out) const
{
	const FDeliveryBBKeyRuntimeV3* KR = Keys.Find(Key);
	if (!KR || KR->Spec.Type != EDeliveryBBValueTypeV3::Name) return false;
	Out = KR->Value.NameValue;
	return true;
}
