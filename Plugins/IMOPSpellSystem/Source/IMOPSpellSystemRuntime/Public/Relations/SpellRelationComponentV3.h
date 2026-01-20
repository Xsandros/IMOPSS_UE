#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Relations/SpellRelationTypesV3.h"
#include "SpellRelationComponentV3.generated.h"

UCLASS(ClassGroup=(IMOP), meta=(BlueprintSpawnableComponent))
class IMOPSPELLSYSTEMRUNTIME_API USpellRelationComponentV3 : public UActorComponent
{
	GENERATED_BODY()
public:
	USpellRelationComponentV3();

	// Generic “who am I aligned with?” (Faction.*, Party.*, Guild.*, etc.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="Relations")
	FGameplayTagContainer AffiliationTags;

	// Ownership/Summons: if true and OwnerActor == Caster, treat as Ally
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="Relations")
	bool bUseOwnerAsAlly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="Relations")
	TObjectPtr<AActor> OwnerActor = nullptr;

	// Fast testing + mind control style override (applies against the *current caster*)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="Relations")
	ESpellForcedRelationV3 ForcedRelationToCaster = ESpellForcedRelationV3::None;

	// Optional global targetable toggle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="Relations")
	bool bTargetable = true;

	UFUNCTION(BlueprintCallable, Category="IMOP|Relations")
	void AddAffiliationTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="IMOP|Relations")
	void RemoveAffiliationTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="IMOP|Relations")
	void ClearAffiliationTags();

	UFUNCTION(BlueprintCallable, Category="IMOP|Relations")
	void SetOwnerActor(AActor* InOwner);

	UFUNCTION(BlueprintCallable, Category="IMOP|Relations")
	void SetForcedRelationToCaster(ESpellForcedRelationV3 Forced);

	UFUNCTION(BlueprintCallable, Category="IMOP|Relations")
	void SetTargetable(bool bInTargetable);

	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
