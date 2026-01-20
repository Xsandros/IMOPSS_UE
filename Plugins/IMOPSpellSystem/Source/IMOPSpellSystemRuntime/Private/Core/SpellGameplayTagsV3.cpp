#include "Core/SpellGameplayTagsV3.h"
#include "GameplayTagsManager.h"

static FGameplayTag RequestTagChecked(const TCHAR* TagName)
{
	// true = error if not found (helps catch missing tags early)
	return UGameplayTagsManager::Get().RequestGameplayTag(FName(TagName), /*ErrorIfNotFound*/ true);
}

const FIMOPSpellGameplayTagsV3& FIMOPSpellGameplayTagsV3::Get()
{
	static FIMOPSpellGameplayTagsV3 Instance;
	if (!Instance.bInitialized)
	{
		Instance.Initialize();
		Instance.bInitialized = true;
	}
	return Instance;
}


void FIMOPSpellGameplayTagsV3::Initialize()
{
	Spell = RequestTagChecked(TEXT("Spell"));
	Action = RequestTagChecked(TEXT("Spell.Action"));
	Event = RequestTagChecked(TEXT("Spell.Event"));

	Action_EmitEvent = RequestTagChecked(TEXT("Spell.Action.EmitEvent"));
	Action_SetVariable = RequestTagChecked(TEXT("Spell.Action.SetVariable"));
	Action_ModifyVariable = RequestTagChecked(TEXT("Spell.Action.ModifyVariable"));
	Action_SetTargetSet = RequestTagChecked(TEXT("Spell.Action.SetTargetSet"));
	Action_ClearTargetSet = RequestTagChecked(TEXT("Spell.Action.ClearTargetSet"));
	Action_Targeting_Acquire = RequestTagChecked(TEXT("Spell.Action.Targeting.Acquire"));
	Action_Targeting_Filter  = RequestTagChecked(TEXT("Spell.Action.Targeting.Filter"));
	Action_Targeting_Select  = RequestTagChecked(TEXT("Spell.Action.Targeting.Select"));

	Action_Effect_ModifyAttribute = RequestTagChecked(TEXT("Spell.Action.Effect.ModifyAttribute"));
	Action_Effect_ReadAttribute   = RequestTagChecked(TEXT("Spell.Action.Effect.ReadAttribute"));
	Action_Effect_ApplyForce      = RequestTagChecked(TEXT("Spell.Action.Effect.ApplyForce"));

	Action_Status_Apply           = RequestTagChecked(TEXT("Spell.Action.Status.Apply"));
	Action_Status_Remove          = RequestTagChecked(TEXT("Spell.Action.Status.Remove"));

	

	Event_Spell_Start = RequestTagChecked(TEXT("Spell.Event.Spell.Start"));
	Event_Spell_End = RequestTagChecked(TEXT("Spell.Event.Spell.End"));
	
	// ===== Phase 3: Effect events =====
	Event_Effect_Attempt   = RequestTagChecked(TEXT("Spell.Event.Effect.Attempt"));
	Event_Effect_Applied   = RequestTagChecked(TEXT("Spell.Event.Effect.Applied"));
	Event_Effect_Blocked   = RequestTagChecked(TEXT("Spell.Event.Effect.Blocked"));
	Event_Effect_Immune    = RequestTagChecked(TEXT("Spell.Event.Effect.Immune"));
	Event_Effect_Resisted  = RequestTagChecked(TEXT("Spell.Event.Effect.Resisted"));
	Event_Effect_Failed    = RequestTagChecked(TEXT("Spell.Event.Effect.Failed"));

	// ===== Phase 3: Status events =====
	Event_Status_Applied   = RequestTagChecked(TEXT("Spell.Event.Status.Applied"));
	Event_Status_Removed   = RequestTagChecked(TEXT("Spell.Event.Status.Removed"));
	Event_Status_Expired   = RequestTagChecked(TEXT("Spell.Event.Status.Expired"));

	// Optional semantic buckets
	Event_Damage_Dealt     = RequestTagChecked(TEXT("Spell.Event.Damage.Dealt"));
	Event_Damage_Taken     = RequestTagChecked(TEXT("Spell.Event.Damage.Taken"));
	Event_Heal_Dealt       = RequestTagChecked(TEXT("Spell.Event.Heal.Dealt"));
	Event_Heal_Received    = RequestTagChecked(TEXT("Spell.Event.Heal.Received"));
	
	// ===== Relations Debug =====
	Event_Relation_Resolved            = RequestTagChecked(TEXT("Spell.Event.Relation.Resolved"));

	Relation_Reason_Forced             = RequestTagChecked(TEXT("Spell.Relation.Reason.Forced"));
	Relation_Reason_OwnerAlly          = RequestTagChecked(TEXT("Spell.Relation.Reason.OwnerAlly"));
	Relation_Reason_SharedAffiliation  = RequestTagChecked(TEXT("Spell.Relation.Reason.SharedAffiliation"));
	Relation_Reason_RuleHostile        = RequestTagChecked(TEXT("Spell.Relation.Reason.RuleHostile"));
	Relation_Reason_RuleFriendly       = RequestTagChecked(TEXT("Spell.Relation.Reason.RuleFriendly"));
	Relation_Reason_TargetNotTargetable= RequestTagChecked(TEXT("Spell.Relation.Reason.TargetNotTargetable"));
	Relation_Reason_FallbackNeutral    = RequestTagChecked(TEXT("Spell.Relation.Reason.FallbackNeutral"));

	Action_Delivery_Start = RequestTagChecked(TEXT("Spell.Action.Delivery.Start"));
	Action_Delivery_Stop  = RequestTagChecked(TEXT("Spell.Action.Delivery.Stop"));

	Event_Delivery_Started = RequestTagChecked(TEXT("Spell.Event.Delivery.Started"));
	Event_Delivery_Stopped = RequestTagChecked(TEXT("Spell.Event.Delivery.Stopped"));
	Event_Delivery_Hit     = RequestTagChecked(TEXT("Spell.Event.Delivery.Hit"));
	Event_Delivery_Enter   = RequestTagChecked(TEXT("Spell.Event.Delivery.Enter"));
	Event_Delivery_Stay    = RequestTagChecked(TEXT("Spell.Event.Delivery.Stay"));
	Event_Delivery_Exit    = RequestTagChecked(TEXT("Spell.Event.Delivery.Exit"));
	Event_Delivery_Tick    = RequestTagChecked(TEXT("Spell.Event.Delivery.Tick"));

}


