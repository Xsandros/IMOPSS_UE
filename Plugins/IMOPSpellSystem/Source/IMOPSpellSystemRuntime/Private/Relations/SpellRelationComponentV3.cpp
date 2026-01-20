#include "Relations/SpellRelationComponentV3.h"
#include "Net/UnrealNetwork.h"

USpellRelationComponentV3::USpellRelationComponentV3()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USpellRelationComponentV3::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USpellRelationComponentV3, AffiliationTags);
	DOREPLIFETIME(USpellRelationComponentV3, bUseOwnerAsAlly);
	DOREPLIFETIME(USpellRelationComponentV3, OwnerActor);
	DOREPLIFETIME(USpellRelationComponentV3, ForcedRelationToCaster);
	DOREPLIFETIME(USpellRelationComponentV3, bTargetable);
}


void USpellRelationComponentV3::AddAffiliationTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		AffiliationTags.AddTag(Tag);
	}
}

void USpellRelationComponentV3::RemoveAffiliationTag(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		AffiliationTags.RemoveTag(Tag);
	}
}

void USpellRelationComponentV3::ClearAffiliationTags()
{
	AffiliationTags.Reset();
}

void USpellRelationComponentV3::SetOwnerActor(AActor* InOwner)
{
	OwnerActor = InOwner;
}

void USpellRelationComponentV3::SetForcedRelationToCaster(ESpellForcedRelationV3 Forced)
{
	ForcedRelationToCaster = Forced;
}

void USpellRelationComponentV3::SetTargetable(bool bInTargetable)
{
	bTargetable = bInTargetable;
}
