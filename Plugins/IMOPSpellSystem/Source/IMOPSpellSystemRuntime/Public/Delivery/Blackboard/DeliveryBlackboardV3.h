#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliverySpecV3.h"
#include "DeliveryBlackboardV3.generated.h"

/**
 * Minimal but future-proof blackboard:
 * - Keys must be declared via InitFromSpec (deterministic, reject unknown keys by default)
 * - Phase enforcement (WritePhaseMask)
 * - Owner enforcement (Group/Primitive/System)
 *
 * This is intentionally not a generic "Variant map" yet; we add types as needed.
 */
USTRUCT(BlueprintType)
struct FDeliveryBBValueV3
{
	GENERATED_BODY()

	UPROPERTY()
	float FloatValue = 0.f;

	UPROPERTY()
	int32 IntValue = 0;

	UPROPERTY()
	bool BoolValue = false;

	UPROPERTY()
	FVector VectorValue = FVector::ZeroVector;

	UPROPERTY()
	FRotator RotatorValue = FRotator::ZeroRotator;

	UPROPERTY()
	FTransform TransformValue = FTransform::Identity;

	UPROPERTY()
	FName NameValue = NAME_None;
};

USTRUCT(BlueprintType)
struct FDeliveryBBKeyRuntimeV3
{
	GENERATED_BODY()

	UPROPERTY()
	FDeliveryBBKeySpecV3 Spec;

	UPROPERTY()
	FDeliveryBBValueV3 Value;
};

USTRUCT(BlueprintType)
struct FDeliveryBlackboardV3
{
	GENERATED_BODY()

public:
	void InitFromSpec(const FDeliveryBlackboardInitSpecV3& Init, const FDeliveryOwnershipRulesV3& Rules);

	void BeginPhase(EDeliveryBBPhaseV3 Phase);

	// Writes (return false if rejected)
	bool WriteFloat(FName Key, float V, EDeliveryBBOwnerV3 Writer);
	bool WriteInt(FName Key, int32 V, EDeliveryBBOwnerV3 Writer);
	bool WriteBool(FName Key, bool V, EDeliveryBBOwnerV3 Writer);
	bool WriteVector(FName Key, const FVector& V, EDeliveryBBOwnerV3 Writer);
	bool WriteRotator(FName Key, const FRotator& V, EDeliveryBBOwnerV3 Writer);
	bool WriteTransform(FName Key, const FTransform& V, EDeliveryBBOwnerV3 Writer);
	bool WriteName(FName Key, FName V, EDeliveryBBOwnerV3 Writer);

	// Reads (return false if missing/type mismatch)
	bool ReadFloat(FName Key, float& Out) const;
	bool ReadInt(FName Key, int32& Out) const;
	bool ReadBool(FName Key, bool& Out) const;
	bool ReadVector(FName Key, FVector& Out) const;
	bool ReadRotator(FName Key, FRotator& Out) const;
	bool ReadTransform(FName Key, FTransform& Out) const;
	bool ReadName(FName Key, FName& Out) const;

	bool HasKey(FName Key) const;

private:
	bool CanWrite(const FDeliveryBBKeyRuntimeV3& KR, EDeliveryBBValueTypeV3 ExpectedType, EDeliveryBBOwnerV3 Writer) const;
	static uint8 PhaseBit(EDeliveryBBPhaseV3 Phase);

private:
	UPROPERTY()
	TMap<FName, FDeliveryBBKeyRuntimeV3> Keys;

	UPROPERTY()
	FDeliveryOwnershipRulesV3 Rules;

	UPROPERTY()
	EDeliveryBBPhaseV3 CurrentPhase = EDeliveryBBPhaseV3::Time;

	UPROPERTY()
	bool bInitialized = false;
};
