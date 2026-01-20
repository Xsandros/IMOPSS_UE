#include "Relations/SpellRelationSubsystemV3.h"
#include "GameplayTagsManager.h"
#include "Core/SpellGameplayTagsV3.h"
#include "Relations/SpellRelationComponentV3.h"
#include "Relations/SpellRelationRulesAssetV3.h"

static const USpellRelationComponentV3* GetRelComp(const AActor* A)
{
	return A ? A->FindComponentByClass<USpellRelationComponentV3>() : nullptr;
}

void USpellRelationSubsystemV3::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Only set defaults if empty (so user/project can override in editor or code)
	if (AffiliationScopes.Num() == 0)
	{
		const UGameplayTagsManager& TM = UGameplayTagsManager::Get();

		auto MakeScope = [&](const TCHAR* RootStr)
		{
			FAffiliationScopeV3 S;
			S.ScopeRoot = TM.RequestGameplayTag(FName(RootStr), /*ErrorIfNotFound*/false);
			S.Match = EAffiliationScopeMatchV3::ExactTag;
			S.bEnabled = S.ScopeRoot.IsValid();
			return S;
		};

		// Priority: Party > Guild > Faction
		AffiliationScopes.Add(MakeScope(TEXT("Affiliation.Party")));
		AffiliationScopes.Add(MakeScope(TEXT("Affiliation.Guild")));
		AffiliationScopes.Add(MakeScope(TEXT("Affiliation.Faction")));
	}
}


bool USpellRelationSubsystemV3::MatchesPair(
	const FGameplayTagContainer& CasterTags,
	const FGameplayTagContainer& TargetTags,
	const FSpellRelationRulePairV3& Pair) const
{
	if (!Pair.A.IsValid() || !Pair.B.IsValid()) return false;

	const bool AB = CasterTags.HasTag(Pair.A) && TargetTags.HasTag(Pair.B);
	const bool BA = CasterTags.HasTag(Pair.B) && TargetTags.HasTag(Pair.A);
	return AB || BA;
}

ETargetRelationV3 USpellRelationSubsystemV3::ResolveRelation(const AActor* Caster, const AActor* Target) const
{
	if (!Caster || !Target) return ETargetRelationV3::Any;

	if (Caster == Target) return ETargetRelationV3::Self;

	const USpellRelationComponentV3* CRel = GetRelComp(Caster);
	const USpellRelationComponentV3* TRel = GetRelComp(Target);

	// If target isn’t targetable, treat as Neutral (filter can drop it if needed)
	if (TRel && !TRel->bTargetable)
	{
		return ETargetRelationV3::Neutral;
	}

	// 1) Forced override (mind control / debug)
	if (TRel)
	{
		switch (TRel->ForcedRelationToCaster)
		{
			case ESpellForcedRelationV3::ForceAlly:   return ETargetRelationV3::Ally;
			case ESpellForcedRelationV3::ForceEnemy:  return ETargetRelationV3::Enemy;
			case ESpellForcedRelationV3::ForceNeutral:return ETargetRelationV3::Neutral;
			default: break;
		}
	}

	// 2) Ownership/Summons: OwnerActor == Caster => Ally
	if (TRel && TRel->bUseOwnerAsAlly && TRel->OwnerActor && TRel->OwnerActor == Caster)
	{
		return ETargetRelationV3::Ally;
	}

	// 3) Shared affiliation tags => Ally (Party/Guild/Faction/etc.)
	if (CRel && TRel)
	{
		// Any overlap => Ally (powerful default; can be refined later via tag namespaces)
		FGameplayTagContainer Intersect = CRel->AffiliationTags;
		Intersect.AppendTags(TRel->AffiliationTags); // temp
		// We want overlap check, not union:
		for (const FGameplayTag& T : CRel->AffiliationTags)
		{
			if (TRel->AffiliationTags.HasTag(T))
			{
				return ETargetRelationV3::Ally;
			}
		}
	}

	// 4) Rules asset hostility/friendly matrix (Faction diplomacy)
	if (RulesAsset && CRel && TRel)
	{
		for (const FSpellRelationRulePairV3& P : RulesAsset->HostilePairs)
		{
			if (MatchesPair(CRel->AffiliationTags, TRel->AffiliationTags, P))
			{
				return ETargetRelationV3::Enemy;
			}
		}
		for (const FSpellRelationRulePairV3& P : RulesAsset->FriendlyPairs)
		{
			if (MatchesPair(CRel->AffiliationTags, TRel->AffiliationTags, P))
			{
				return ETargetRelationV3::Ally;
			}
		}
	}

	// Fallback
	return ETargetRelationV3::Neutral;
}

void USpellRelationSubsystemV3::GatherTagsUnderRoot(const FGameplayTagContainer& Source, FGameplayTag Root, TArray<FGameplayTag>& Out)
{
	Out.Reset();
	if (!Root.IsValid()) return;

	for (const FGameplayTag& T : Source)
	{
		if (T.IsValid() && T.MatchesTag(Root))
		{
			Out.Add(T);
		}
	}
}

bool USpellRelationSubsystemV3::FindExactOverlap(const TArray<FGameplayTag>& A, const TArray<FGameplayTag>& B, FGameplayTag& OutMatched)
{
	for (const FGameplayTag& TA : A)
	{
		for (const FGameplayTag& TB : B)
		{
			if (TA == TB)
			{
				OutMatched = TA;
				return true;
			}
		}
	}
	return false;
}

static void GatherTagsUnderRoot(const FGameplayTagContainer& Source, FGameplayTag Root, TArray<FGameplayTag>& Out)
{
	Out.Reset();
	if (!Root.IsValid()) return;

	for (const FGameplayTag& T : Source)
	{
		if (T.IsValid() && T.MatchesTag(Root))
		{
			Out.Add(T);
		}
	}
}

static bool FindExactOverlap(const TArray<FGameplayTag>& A, const TArray<FGameplayTag>& B, FGameplayTag& OutMatched)
{
	for (const FGameplayTag& TA : A)
	{
		for (const FGameplayTag& TB : B)
		{
			if (TA == TB)
			{
				OutMatched = TA;
				return true;
			}
		}
	}
	return false;
}


ETargetRelationV3 USpellRelationSubsystemV3::ResolveRelationDetailed(
	const AActor* Caster,
	const AActor* Target,
	FGameplayTag& OutReasonTag,
	FGameplayTag& OutMatchedA,
	FGameplayTag& OutMatchedB) const
{
	OutReasonTag = FGameplayTag();
	OutMatchedA = FGameplayTag();
	OutMatchedB = FGameplayTag();

	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

	if (!Caster || !Target)
	{
		return ETargetRelationV3::Any;
	}

	if (Caster == Target)
	{
		// not really used (Self handled earlier), but ok
		return ETargetRelationV3::Self;
	}

	const USpellRelationComponentV3* CRel = GetRelComp(Caster);
	const USpellRelationComponentV3* TRel = GetRelComp(Target);

	if (TRel && !TRel->bTargetable)
	{
		OutReasonTag = Tags.Relation_Reason_TargetNotTargetable;
		return ETargetRelationV3::Neutral;
	}

	// Forced override
	if (TRel)
	{
		switch (TRel->ForcedRelationToCaster)
		{
		case ESpellForcedRelationV3::ForceAlly:
			OutReasonTag = Tags.Relation_Reason_Forced;
			return ETargetRelationV3::Ally;
		case ESpellForcedRelationV3::ForceEnemy:
			OutReasonTag = Tags.Relation_Reason_Forced;
			return ETargetRelationV3::Enemy;
		case ESpellForcedRelationV3::ForceNeutral:
			OutReasonTag = Tags.Relation_Reason_Forced;
			return ETargetRelationV3::Neutral;
		default: break;
		}
	}

	// Ownership
	if (TRel && TRel->bUseOwnerAsAlly && TRel->OwnerActor && TRel->OwnerActor == Caster)
	{
		OutReasonTag = Tags.Relation_Reason_OwnerAlly;
		return ETargetRelationV3::Ally;
	}

	// 3) Shared affiliation by scoped priority => Ally
	if (CRel && TRel && AffiliationScopes.Num() > 0)
	{
		TArray<FGameplayTag> CTags;
		TArray<FGameplayTag> TTags;

		for (const FAffiliationScopeV3& Scope : AffiliationScopes)
		{
			if (!Scope.bEnabled || !Scope.ScopeRoot.IsValid())
			{
				continue;
			}

			GatherTagsUnderRoot(CRel->AffiliationTags, Scope.ScopeRoot, CTags);
			GatherTagsUnderRoot(TRel->AffiliationTags, Scope.ScopeRoot, TTags);

			if (CTags.Num() == 0 || TTags.Num() == 0)
			{
				continue;
			}

			if (Scope.Match == EAffiliationScopeMatchV3::AnyUnderRoot)
			{
				OutReasonTag = Tags.Relation_Reason_SharedAffiliation;
				OutMatchedA = Scope.ScopeRoot;
				OutMatchedB = Scope.ScopeRoot;
				return ETargetRelationV3::Ally;
			}

			FGameplayTag Matched;
			if (FindExactOverlap(CTags, TTags, Matched))
			{
				OutReasonTag = Tags.Relation_Reason_SharedAffiliation;
				OutMatchedA = Matched;
				OutMatchedB = Matched;
				return ETargetRelationV3::Ally;
			}
		}
	}


	// Rules asset
	if (RulesAsset && CRel && TRel)
	{
		for (const FSpellRelationRulePairV3& P : RulesAsset->HostilePairs)
		{
			if (MatchesPair(CRel->AffiliationTags, TRel->AffiliationTags, P))
			{
				OutReasonTag = Tags.Relation_Reason_RuleHostile;
				OutMatchedA = P.A;
				OutMatchedB = P.B;
				return ETargetRelationV3::Enemy;
			}
		}
		for (const FSpellRelationRulePairV3& P : RulesAsset->FriendlyPairs)
		{
			if (MatchesPair(CRel->AffiliationTags, TRel->AffiliationTags, P))
			{
				OutReasonTag = Tags.Relation_Reason_RuleFriendly;
				OutMatchedA = P.A;
				OutMatchedB = P.B;
				return ETargetRelationV3::Ally;
			}
		}
	}

	OutReasonTag = Tags.Relation_Reason_FallbackNeutral;
	return ETargetRelationV3::Neutral;
}


