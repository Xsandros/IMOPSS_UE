#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SpellAttributeComponentV3.generated.h"

// One replicated attribute entry
USTRUCT()
struct FReplicatedAttributeEntryV3 : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Key;

	UPROPERTY()
	float Value = 0.f;
};

// The replicated container
USTRUCT()
struct FReplicatedAttributeContainerV3 : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FReplicatedAttributeEntryV3> Items;

	// Lookup helpers (not replicated, runtime only)
	bool FindValue(FGameplayTag Key, float& OutValue) const
	{
		if (!Key.IsValid()) { OutValue = 0.f; return false; }
		for (const FReplicatedAttributeEntryV3& E : Items)
		{
			if (E.Key == Key) { OutValue = E.Value; return true; }
		}
		OutValue = 0.f;
		return false;
	}

	// Sets or adds; marks dirty for replication
	void SetValue(FGameplayTag Key, float NewValue)
	{
		if (!Key.IsValid()) return;

		for (FReplicatedAttributeEntryV3& E : Items)
		{
			if (E.Key == Key)
			{
				E.Value = NewValue;
				MarkItemDirty(E);
				return;
			}
		}

		FReplicatedAttributeEntryV3& NewEntry = Items.AddDefaulted_GetRef();
		NewEntry.Key = Key;
		NewEntry.Value = NewValue;
		MarkItemDirty(NewEntry);
	}

	// Required for FastArray replication
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FReplicatedAttributeEntryV3, FReplicatedAttributeContainerV3>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FReplicatedAttributeContainerV3> : public TStructOpsTypeTraitsBase2<FReplicatedAttributeContainerV3>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

UCLASS(ClassGroup=(IMOP), meta=(BlueprintSpawnableComponent))
class IMOPSPELLSYSTEMRUNTIME_API USpellAttributeComponentV3 : public UActorComponent
{
	GENERATED_BODY()

public:
	USpellAttributeComponentV3();

	UFUNCTION(BlueprintCallable, Category="IMOP|Attributes")
	bool GetAttribute(FGameplayTag AttributeTag, float& OutValue) const;

	UFUNCTION(BlueprintCallable, Category="IMOP|Attributes")
	void SetAttribute(FGameplayTag AttributeTag, float NewValue);

	UFUNCTION(BlueprintCallable, Category="IMOP|Attributes")
	void AddDelta(FGameplayTag AttributeTag, float Delta, float& OutNewValue);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// Replicated via FastArraySerializer (supported)
	UPROPERTY(Replicated)
	FReplicatedAttributeContainerV3 ReplicatedAttributes;
};
