#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliveryTypesV3.h"
#include "DeliveryRigNodeV3.generated.h"

UENUM(BlueprintType)
enum class EDeliveryRigNodeKindV3 : uint8
{
	// Produces a base pose from an attachment source
	Attach,

	// Applies local offset/rotation on parent pose
	Offset,

	// Reorients rotation (keeps location)
	AimForward,      // aims forward based on parent + local rotation
	LookAtTargetSet, // aims at center of a target set

	// Reorients rotation over time (general purpose: scanning, spinning, sweeping)
	RotateOverTime, // rotation rate * elapsed

	// Produces multiple emitter poses around current pose (does not change root)
	OrbitSampler,

	// Adds deterministic noise to root and/or emitters
	Jitter,

};

USTRUCT(BlueprintType)
struct FDeliveryRigNodeV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	EDeliveryRigNodeKindV3 Kind = EDeliveryRigNodeKindV3::Attach;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	int32 Parent = INDEX_NONE;

	// Common local transform adjustments (used by Offset/AimForward etc.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FRotator LocalRotation = FRotator::ZeroRotator;

	// Attach source
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FDeliveryAttachV3 Attach;

	// LookAt target set key (uses Attach.TargetSetName if empty)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FName LookAtTargetSet = NAME_None;

	// Orbit sampler params
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	int32 OrbitCount = 0; // 0 = disabled

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	float OrbitRadius = 0.f;

	// base phase (degrees)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig") 
	float OrbitPhaseDegrees = 0.f; 

	// Time-awareness: additional phase = ElapsedSeconds * OrbitAngularSpeedDegPerSec
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig") 
	float OrbitAngularSpeedDegPerSec = 0.f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FVector OrbitAxis = FVector::UpVector; // ring plane normal

	// Jitter params
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	float JitterRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	float JitterYawDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	bool bApplyJitterToRoot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig") 
	bool bApplyJitterToEmitters = true;

	// RotateOverTime params (degrees per second)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FRotator RotationRateDegPerSec = FRotator::ZeroRotator;
	
	// Spawn slot used by this node when producing emitters.
	// DeliverySubsystem can map SpawnSlot -> SlotOverrides[] to vary kind/shape/tags/config per emitter.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	int32 SpawnSlot = 0;

	// If true, orbit emitters will cycle spawn slots: Slot = SpawnSlot + (i % OrbitSlotModulo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	bool bOrbitSlotByIndex = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="bOrbitSlotByIndex"))
	int32 OrbitSlotModulo = 0; // 0 => disabled

	
};
