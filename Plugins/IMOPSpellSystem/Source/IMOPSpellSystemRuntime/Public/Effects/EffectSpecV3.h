#pragma once

#include "CoreMinimal.h"
#include "Effects/EffectTypesV3.h"
#include "EffectSpecV3.generated.h"

USTRUCT(BlueprintType)
struct FEffect_ModifyAttributeV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FAttributeKeyV3 Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	EAttributeOpV3 Op = EAttributeOpV3::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FMagnitudeV3 Magnitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	bool bClampToMinMax = false;
};

USTRUCT(BlueprintType)
struct FEffect_ApplyForceV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	EForceKindV3 Kind = EForceKindV3::Impulse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	float Strength = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	bool bIgnoreMass = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	float Radius = 0.f;
};

UENUM(BlueprintType)
enum class EEffectKindV3 : uint8
{
	ModifyAttribute,
	ApplyForce
};

USTRUCT(BlueprintType)
struct FEffectSpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	EEffectKindV3 Kind = EEffectKindV3::ModifyAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FEffectContextV3 Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FEffect_ModifyAttributeV3 ModifyAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FEffect_ApplyForceV3 ApplyForce;
};
