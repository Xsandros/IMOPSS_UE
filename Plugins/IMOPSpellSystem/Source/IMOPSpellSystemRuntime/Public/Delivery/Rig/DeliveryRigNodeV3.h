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
	
	// Produces multiple emitters in a fan / arc (shotgun, scanning wedge)
	FanSampler,

// Produces multiple emitters via deterministic scatter inside a volume
	ScatterSampler
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

	// If true, this node also applies to any already-produced emitters (in addition to root).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	bool bAffectEmitters = false;

	
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

	// If true, OrbitSampler appends its emitters to the current emitter list.
	// If false, it replaces the current emitters.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	bool bAppendEmitters = true;
	
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
	
	// ===== Fan sampler params =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	int32 FanCount = 0; // 0 => disabled

	// Total arc in degrees (centered around forward)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="FanCount>0"))
	float FanArcDegrees = 30.f;

	// Local axis to fan around (default Up = yaw fan)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="FanCount>0"))
	FVector FanAxis = FVector::UpVector;

	// Distance from root along each direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="FanCount>0"))
	float FanDistance = 1000.f;

	// Base slot and optional cycling like OrbitSampler
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="FanCount>0"))
	bool bFanSlotByIndex = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="bFanSlotByIndex"))
	int32 FanSlotModulo = 0;

	// ===== Scatter sampler params =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	int32 ScatterCount = 0; // 0 => disabled

	// Scatter volume type: sphere if Radius>0, otherwise box if Extents non-zero
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="ScatterCount>0"))
	float ScatterRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="ScatterCount>0"))
	FVector ScatterExtents = FVector::ZeroVector;

	// Optional: randomize yaw for each point
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="ScatterCount>0"))
	float ScatterYawDegrees = 0.f;

	// Slot cycling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="ScatterCount>0"))
	bool bScatterSlotByIndex = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(EditCondition="bScatterSlotByIndex"))
	int32 ScatterSlotModulo = 0;


	
};
