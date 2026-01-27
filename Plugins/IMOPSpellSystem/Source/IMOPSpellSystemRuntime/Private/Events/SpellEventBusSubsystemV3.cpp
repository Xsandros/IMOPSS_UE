#include "Events/SpellEventBusSubsystemV3.h"

#include "UObject/UObjectBaseUtility.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPSpellEventBusV3, Log, All);

void USpellEventBusSubsystemV3::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Subscribers.Reset();
	NextId = 1;

	UE_LOG(LogIMOPSpellEventBusV3, Log, TEXT("SpellEventBus initialized"));
}

void USpellEventBusSubsystemV3::Deinitialize()
{
	Subscribers.Reset();

	UE_LOG(LogIMOPSpellEventBusV3, Log, TEXT("SpellEventBus deinitialized"));

	Super::Deinitialize();
}

bool USpellEventBusSubsystemV3::MatchesFilter(const FGameplayTag& EventTag, const FGameplayTag& FilterTag)
{
	if (!FilterTag.IsValid())
	{
		return true; // receive all
	}

	// MatchesTag handles hierarchical tags (e.g. A.B.C matches A or A.B if desired)
	return EventTag.IsValid() && (EventTag.MatchesTag(FilterTag) || FilterTag.MatchesTag(EventTag));
}

ISpellEventListenerV3* USpellEventBusSubsystemV3::AsListener(UObject* Obj)
{
	if (!Obj)
	{
		return nullptr;
	}

	// Two supported patterns:
	// 1) Obj is a UObject implementing the UInterface (Blueprint/C++)
	// 2) Obj is a C++ type where we can cast to ISpellEventListenerV3 via interface address
	if (Obj->GetClass()->ImplementsInterface(USpellEventListenerV3::StaticClass()))
	{
		return Cast<ISpellEventListenerV3>(Obj);
	}

	// Fallback: try direct cast (works if the type is known in C++)
	return Cast<ISpellEventListenerV3>(Obj);
}

FSpellEventSubscriptionHandleV3 USpellEventBusSubsystemV3::Subscribe(UObject* ListenerObj, FGameplayTag TagFilter)
{
	FSpellEventSubscriptionHandleV3 Handle;

	if (!ListenerObj)
	{
		UE_LOG(LogIMOPSpellEventBusV3, Warning, TEXT("Subscribe rejected: ListenerObj null"));
		return Handle;
	}

	if (!AsListener(ListenerObj))
	{
		UE_LOG(LogIMOPSpellEventBusV3, Warning, TEXT("Subscribe rejected: ListenerObj does not implement ISpellEventListenerV3 (%s)"),
			*ListenerObj->GetName());
		return Handle;
	}

	FSpellEventBusSubscriberV3 S;
	S.Id = NextId++;
	S.ListenerObj = ListenerObj;
	S.TagFilter = TagFilter;

	Subscribers.Add(S);

	Handle.Id = S.Id;

	UE_LOG(LogIMOPSpellEventBusV3, Log, TEXT("Subscribed id=%d obj=%s filter=%s subs=%d"),
		S.Id,
		*ListenerObj->GetName(),
		TagFilter.IsValid() ? *TagFilter.ToString() : TEXT("<ALL>"),
		Subscribers.Num());

	return Handle;
}

FSpellEventSubscriptionHandleV3 USpellEventBusSubsystemV3::Subscribe(ISpellEventListenerV3* Listener, FGameplayTag TagFilter)
{
	// We need the UObject to store weak pointer.
	UObject* Obj = Cast<UObject>(Listener);
	return Subscribe(Obj, TagFilter);
}

void USpellEventBusSubsystemV3::Unsubscribe(const FSpellEventSubscriptionHandleV3& Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	const int32 Removed = Subscribers.RemoveAll([&](const FSpellEventBusSubscriberV3& S)
	{
		return S.Id == Handle.Id;
	});

	if (Removed > 0)
	{
		UE_LOG(LogIMOPSpellEventBusV3, Log, TEXT("Unsubscribed id=%d (removed=%d) subs=%d"),
			Handle.Id, Removed, Subscribers.Num());
	}
}

void USpellEventBusSubsystemV3::Emit(const FSpellEventV3& Ev)
{
	// Compact dead subscribers occasionally
	Subscribers.RemoveAll([](const FSpellEventBusSubscriberV3& S)
	{
		return !S.ListenerObj.IsValid();
	});

	for (const FSpellEventBusSubscriberV3& S : Subscribers)
	{
		UObject* Obj = S.ListenerObj.Get();
		if (!Obj)
		{
			continue;
		}

		if (!MatchesFilter(Ev.EventTag, S.TagFilter))
		{
			continue;
		}

		if (ISpellEventListenerV3* L = AsListener(Obj))
		{
			L->OnSpellEvent(Ev);
		}
	}
}
