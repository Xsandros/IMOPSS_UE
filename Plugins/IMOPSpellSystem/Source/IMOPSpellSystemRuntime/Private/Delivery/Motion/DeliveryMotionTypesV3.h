#pragma once

#include "CoreMinimal.h"
#include "InstancedStruct.h"
#include "StructUtils/InstancedStruct.h"
#include "DeliveryMotionTypesV3.generated.h"

UENUM(BlueprintType)
enum class EDeliveryMotionSpaceV3 : uint8
{
    Local,
    Anchor,
    World
};

UENUM(BlueprintType)
enum class EDeliveryMotionApplyPolicyV3 : uint8
{
    Multiply,        // default: full transform multiply
    TranslateOnly,
    RotateOnly,
    ScaleOnly,
    TranslateRotate,
    TranslateScale,
    RotateScale
};

USTRUCT(BlueprintType)
struct FDeliveryMotionSpecV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    bool bEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    EDeliveryMotionSpaceV3 Space = EDeliveryMotionSpaceV3::Local;

    // This is your "World Space TranslateOnly option" knob.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    EDeliveryMotionApplyPolicyV3 ApplyPolicy = EDeliveryMotionApplyPolicyV3::Multiply;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float Weight = 1.0f;

    // Variant payload: one of the payload structs below.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FInstancedStruct Payload;
};

// -------------------------------
// Payloads (sensible & deterministic)
// -------------------------------

// 1) Linear velocity (classic projectile drift)
USTRUCT(BlueprintType)
struct FDeliveryMotion_LinearVelocityV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FVector Velocity = FVector(1000.f, 0.f, 0.f);

    // If true, interpret Velocity in the chosen Space basis (Local/Anchor/World).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    bool bVelocityInSpaceBasis = true;
};

// 2) Constant acceleration (gravity-like, thrust, etc.)
USTRUCT(BlueprintType)
struct FDeliveryMotion_AccelerationV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FVector InitialVelocity = FVector(1000.f, 0.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FVector Acceleration = FVector(0.f, 0.f, -980.f); // cm/s^2
};

// 3) Orbit around origin (anchor or world)
USTRUCT(BlueprintType)
struct FDeliveryMotion_OrbitV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float Radius = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float AngularSpeedDeg = 180.f; // degrees/sec

    // Which axis to orbit around (in chosen Space basis)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FVector Axis = FVector(0.f, 0.f, 1.f);

    // Optional phase offset per primitive (degrees)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float PhaseDeg = 0.f;
};

// 4) Spin (rotate in place)
USTRUCT(BlueprintType)
struct FDeliveryMotion_SpinV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FRotator AngularVelocityDeg = FRotator(0.f, 360.f, 0.f); // deg/sec
};

// 5) Sine offset (bob/wave)
USTRUCT(BlueprintType)
struct FDeliveryMotion_SineOffsetV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FVector Amplitude = FVector(0.f, 0.f, 30.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float FrequencyHz = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float Phase = 0.f; // radians
};

// 6) Sine scale (pulse)
USTRUCT(BlueprintType)
struct FDeliveryMotion_SineScaleV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float Amplitude = 0.2f; // +/- around 1

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float FrequencyHz = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float Phase = 0.f;
};

// 7) Deterministic "noise" offset (pseudo-noise)
USTRUCT(BlueprintType)
struct FDeliveryMotion_NoiseOffsetV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FVector Amplitude = FVector(10.f, 10.f, 10.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float FrequencyHz = 2.0f;
};

// 8) Timed lerp offset (good for swings/arcs without spline)
UENUM(BlueprintType)
enum class EDeliveryEaseV3 : uint8 { Linear, SmoothStep };

USTRUCT(BlueprintType)
struct FDeliveryMotion_LerpOffsetV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FVector From = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    FVector To = FVector(200.f, 0.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    float Duration = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
    EDeliveryEaseV3 Ease = EDeliveryEaseV3::SmoothStep;
};
