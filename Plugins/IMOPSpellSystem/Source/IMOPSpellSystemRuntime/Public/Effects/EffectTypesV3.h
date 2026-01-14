#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Attributes/AttributeProviderV3.h"
#include "EffectTypesV3.generated.h"

UENUM(BlueprintType)
enum class EMagnitudeSourceV3 : uint8
{
	Constant,
	FromVariable,
	FromAttribute
};

USTRUCT(BlueprintType)
struct FMagnitudeV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	EMagnitudeSourceV3 Source = EMagnitudeSourceV3::Constant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	float Constant = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FName VariableKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FAttributeKeyV3 Attribute;
};

USTRUCT(BlueprintType)
struct FEffectContextV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	TWeakObjectPtr<AActor> Source;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FGameplayTagContainer SpellTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FGameplayTagContainer EffectTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FGameplayTag HitZoneTag;
};

UENUM(BlueprintType)
enum class EForceKindV3 : uint8
{
	Impulse,
	RadialImpulse,
	Launch
};
