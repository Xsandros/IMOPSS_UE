#include "Actions/SpellActionRegistryV3.h"
#include "Actions/SpellActionExecutorV3.h"

bool USpellActionRegistryV3::RegisterBinding(
	const FGameplayTag& ActionTag,
	TSubclassOf<USpellActionExecutorV3> ExecutorClass,
	const UScriptStruct* PayloadStruct,
	bool bAllowOverride,
	FText* OutError)
{
	if (!ActionTag.IsValid())
	{
		if (OutError) { *OutError = FText::FromString(TEXT("ActionTag is invalid.")); }
		return false;
	}
	if (!ExecutorClass)
	{
		if (OutError) { *OutError = FText::FromString(TEXT("ExecutorClass is null.")); }
		return false;
	}
	if (PayloadStruct == nullptr)
	{
		if (OutError) { *OutError = FText::FromString(TEXT("PayloadStruct is null.")); }
		return false;
	}

	const bool bExists = Bindings.Contains(ActionTag);
	if (bExists && !bAllowOverride)
	{
		if (OutError)
		{
			*OutError = FText::FromString(FString::Printf(TEXT("Binding already exists for ActionTag: %s"), *ActionTag.ToString()));
		}
		return false;
	}

	FRegisteredActionBindingV3 Entry;
	Entry.ActionTag = ActionTag;
	Entry.ExecutorClass = ExecutorClass;
	Entry.PayloadStruct = PayloadStruct;

	Bindings.Add(ActionTag, Entry);
	return true;
}

const FRegisteredActionBindingV3* USpellActionRegistryV3::FindBinding(const FGameplayTag& ActionTag) const
{
	return Bindings.Find(ActionTag);
}

void USpellActionRegistryV3::GetAllBindings(TArray<FRegisteredActionBindingV3>& Out) const
{
	Out.Reset();
	Out.Reserve(Bindings.Num());
	for (const auto& KVP : Bindings)
	{
		Out.Add(KVP.Value);
	}
}
