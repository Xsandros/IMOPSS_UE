#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Canonical access to core gameplay tags used by IMOP Spell System.
 * Keep this small: roots + core actions + core events.
 */
struct IMOPSPELLSYSTEMRUNTIME_API FIMOPSpellGameplayTagsV3
{
	static const FIMOPSpellGameplayTagsV3& Get();

	// Roots
	FGameplayTag Spell;
	FGameplayTag Action;
	FGameplayTag Event;

	// Action roots
	FGameplayTag Action_EmitEvent;
	FGameplayTag Action_SetVariable;
	FGameplayTag Action_ModifyVariable;
	FGameplayTag Action_SetTargetSet;
	FGameplayTag Action_ClearTargetSet;
	
	// Targeting action roots
	FGameplayTag Action_Targeting_Acquire;
	FGameplayTag Action_Targeting_Filter;
	FGameplayTag Action_Targeting_Select;


	// Event roots
	FGameplayTag Event_Spell_Start;
	FGameplayTag Event_Spell_End;

private:
	void Initialize();
	bool bInitialized = false;
};
