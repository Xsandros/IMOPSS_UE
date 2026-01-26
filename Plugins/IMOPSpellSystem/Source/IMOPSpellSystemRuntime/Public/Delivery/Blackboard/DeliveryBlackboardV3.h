#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliverySpecV3.h" // for enums/spec structs

#include "DeliveryBlackboardV3.generated.h"

// Lightweight, deterministic typed blackboard.
// - Keys must be registered (type + owner + write mask).
// - Writes are enforced by phase + owner (strict, MP friendly).

USTRUCT()
struct FDeliveryBBValueV3
{
	GENERATED_BODY()

	EDeliveryBBValueTypeV3 Type = EDeliveryBBValueTypeV3::Float;

	bool BoolValue = false;
	int32 IntValue = 0;
	float FloatValue = 0.f;
	FVector VectorValue = FVector::ZeroVector;
	FRotator RotatorValue = FRotator::ZeroRotator;
	FTransform TransformValue = FTransform::Identity;
};

USTRUCT()
struct FDeliveryBBRegisteredKeyV3
{
	GENERATED_BODY()

	FDeliveryBBKeySpecV3 Spec;
	FDeliveryBBValueV3 Value;
};

USTRUCT()
struct FDeliveryBlackboardV3
{
	GENERATED_BODY()

public:
	void InitFromSpec(const FDeliveryBlackboardInitSpecV3& InitSpec, const FDeliveryOwnershipRulesV3& Rules);

	void BeginPhase(EDeliveryBBPhaseV3 Phase);

	bool HasKey(FName Key) const { return Keys.Contains(Key); }

	// Typed reads (returns default if missing/wrong type)
	bool ReadBool(FName Key, bool DefaultValue=false) const;
	int32 ReadInt(FName Key, int32 DefaultValue=0) const;
	float ReadFloat(FName Key, float DefaultValue=0.f) const;
	FVector ReadVector(FName Key, const FVector& DefaultValue=FVector::ZeroVector) const;
	FRotator ReadRotator(FName Key, const FRotator& DefaultValue=FRotator::ZeroRotator) const;
	FTransform ReadTransform(FName Key, const FTransform& DefaultValue=FTransform::Identity) const;

	// Typed writes (enforced)
	bool WriteBool(FName Key, bool V, EDeliveryBBOwnerV3 Writer);
	bool WriteInt(FName Key, int32 V, EDeliveryBBOwnerV3 Writer);
	bool WriteFloat(FName Key, float V, EDeliveryBBOwnerV3 Writer);
	bool WriteVector(FName Key, const FVector& V, EDeliveryBBOwnerV3 Writer);
	bool WriteRotator(FName Key, const FRotator& V, EDeliveryBBOwnerV3 Writer);
	bool WriteTransform(FName Key, const FTransform& V, EDeliveryBBOwnerV3 Writer);

private:
	bool CanWrite(FName Key, EDeliveryBBValueTypeV3 ExpectedType, EDeliveryBBOwnerV3 Writer) const;
	bool SetValue(FName Key, const FDeliveryBBValueV3& NewValue);

	uint8 PhaseBit(EDeliveryBBPhaseV3 P) const { return uint8(1u << uint8(P)); }

private:
	EDeliveryBBPhaseV3 CurrentPhase = EDeliveryBBPhaseV3::Time;
	FDeliveryOwnershipRulesV3 Rules;

	TMap<FName, FDeliveryBBRegisteredKeyV3> Keys;
};
