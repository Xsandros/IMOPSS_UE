#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SpellPayloadsCoreV3.generated.h"

// =========================
// A) EmitEvent
// =========================
USTRUCT(BlueprintType)
struct FPayload_EmitEventV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FGameplayTag EventTag;
};

// =========================
// B) Variables
// =========================
UENUM(BlueprintType)
enum class ESpellValueTypeV3 : uint8
{
	Bool,
	Int,
	Float,
	Name,
	Tag,
	Object
};

USTRUCT(BlueprintType)
struct FSpellValueV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Value")
	ESpellValueTypeV3 Type = ESpellValueTypeV3::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Value")
	bool Bool = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Value")
	int32 Int = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Value")
	float Float = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Value")
	FName Name = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Value")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Value")
	TObjectPtr<UObject> Object = nullptr;
};

USTRUCT(BlueprintType)
struct FPayload_SetVariableV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FSpellValueV3 Value;
};

UENUM(BlueprintType)
enum class ESpellNumericOpV3 : uint8
{
	Add, Sub, Mul, Div, Min, Max, Set
};

USTRUCT(BlueprintType)
struct FPayload_ModifyVariableV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	ESpellNumericOpV3 Op = ESpellNumericOpV3::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	float Operand = 0.f;
};

// =========================
// C) TargetSets (minimal placeholder)
// =========================
USTRUCT(BlueprintType)
struct FPayload_SetTargetSetV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName TargetSetKey = NAME_None;

	// Placeholder: later proper TargetHandles/Queries
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	TArray<TObjectPtr<AActor>> Actors;
};

USTRUCT(BlueprintType)
struct FPayload_ClearTargetSetV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName TargetSetKey = NAME_None;
};
