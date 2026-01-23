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
	
	// ===== Phase 3: Effects =====
	FGameplayTag Action_Effect_ModifyAttribute;
	FGameplayTag Action_Effect_ReadAttribute;
	FGameplayTag Action_Effect_ApplyForce;

	// ===== Phase 3: Status =====
	FGameplayTag Action_Status_Apply;
	FGameplayTag Action_Status_Remove;

	// ===== Phase 3: Events (recommended for final) =====
	FGameplayTag Event_Effect_Attempt;
	FGameplayTag Event_Effect_Applied;
	FGameplayTag Event_Effect_Blocked;
	FGameplayTag Event_Effect_Immune;
	FGameplayTag Event_Effect_Resisted;
	FGameplayTag Event_Effect_Failed;

	FGameplayTag Event_Status_Applied;
	FGameplayTag Event_Status_Removed;
	FGameplayTag Event_Status_Expired;

	// Optional “semantic” buckets (final tracing)
	FGameplayTag Event_Damage_Dealt;
	FGameplayTag Event_Damage_Taken;
	FGameplayTag Event_Heal_Dealt;
	FGameplayTag Event_Heal_Received;
	
	// ===== Relations Debug (Targeting/GameHooks) =====
	FGameplayTag Event_Relation_Resolved;

	// Reason tags carried inside event Data (not event tags themselves)
	FGameplayTag Relation_Reason_Forced;
	FGameplayTag Relation_Reason_OwnerAlly;
	FGameplayTag Relation_Reason_SharedAffiliation;
	FGameplayTag Relation_Reason_RuleHostile;
	FGameplayTag Relation_Reason_RuleFriendly;
	FGameplayTag Relation_Reason_TargetNotTargetable;
	FGameplayTag Relation_Reason_FallbackNeutral;

	
	// ===== Phase 4: Delivery =====
	FGameplayTag Action_Delivery_Start;
	FGameplayTag Action_Delivery_Stop;

	FGameplayTag Event_Delivery_Started;
	FGameplayTag Event_Delivery_Stopped;
	FGameplayTag Event_Delivery_Hit;
	FGameplayTag Event_Delivery_Enter;
	FGameplayTag Event_Delivery_Stay;
	FGameplayTag Event_Delivery_Exit;
	FGameplayTag Event_Delivery_Tick;
	// Delivery primitive events (per spawned primitive / emitter instance)
	FGameplayTag Event_Delivery_Primitive_Started;
	FGameplayTag Event_Delivery_Primitive_Stopped;
	FGameplayTag Event_Delivery_Primitive_Hit;
	FGameplayTag Event_Delivery_Primitive_Enter;
	FGameplayTag Event_Delivery_Primitive_Stay;
	FGameplayTag Event_Delivery_Primitive_Exit;
	FGameplayTag Event_Delivery_Primitive_Tick;

private:
	void Initialize();
	bool bInitialized = false;
};
