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
}


