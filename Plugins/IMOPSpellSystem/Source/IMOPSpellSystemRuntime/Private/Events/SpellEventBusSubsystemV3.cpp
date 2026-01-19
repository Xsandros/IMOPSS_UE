#include "Events/SpellEventBusSubsystemV3.h"
#include "Events/SpellEventListenerV3.h"
#include "Debug/SpellTraceSubsystemV3.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPEventBusV3, Log, All);

FSpellEventSubscriptionHandleV3 USpellEventBusSubsystemV3::Subscribe(UObject* Listener, const FSpellTriggerMatcherV3& Matcher)
{
    FSpellEventSubscriptionHandleV3 Handle;

    if (!Listener)
    {
        UE_LOG(LogIMOPEventBusV3, Warning, TEXT("Subscribe failed: Listener is null."));
        return Handle;
    }

    if (!Listener->GetClass()->ImplementsInterface(USpellEventListenerV3::StaticClass()))
    {
        UE_LOG(LogIMOPEventBusV3, Error, TEXT("Subscribe failed: Listener %s does not implement SpellEventListenerV3."),
            *Listener->GetName());
        return Handle;
    }

    FSpellEventBusSubscriberV3 S;
    S.Id = NextId++;
    S.Listener = Listener;
    S.Matcher = Matcher;

    Subscribers.Add(S);

    Handle.Id = S.Id;
    return Handle;
}

bool USpellEventBusSubsystemV3::Unsubscribe(const FSpellEventSubscriptionHandleV3& Handle)
{
    if (!Handle.IsValid())
    {
        return false;
    }

    const int32 Removed = Subscribers.RemoveAll([&](const FSpellEventBusSubscriberV3& S)
        {
            return S.Id == Handle.Id;
        });

    return Removed > 0;
}

void USpellEventBusSubsystemV3::Emit(const FSpellEventV3& Ev)
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (USpellTraceSubsystemV3* Trace = GI->GetSubsystem<USpellTraceSubsystemV3>())
        {
            UE_LOG(LogTemp, Warning, TEXT("Emit %s guid=%s sender=%s"),
             *Ev.EventTag.ToString(),
             *Ev.RuntimeGuid.ToString(),
            Ev.Sender ? *Ev.Sender->GetName() : TEXT("None"));

            Trace->Record(Ev);
        }
    }

    
    // Compact dead listeners
    Subscribers.RemoveAll([](const FSpellEventBusSubscriberV3& S)
        {
            return !S.Listener.IsValid();
        });
    
    

    for (const FSpellEventBusSubscriberV3& S : Subscribers)
    {
        UObject* L = S.Listener.Get();
        if (!L) continue;

        if (!S.Matcher.Matches(Ev.EventTag))
        {
            continue;
        }

        ISpellEventListenerV3* AsListener = Cast<ISpellEventListenerV3>(L);
        if (!AsListener)
        {
            // Should not happen if interface check was true, but be robust
            continue;
        }

        AsListener->OnSpellEvent(Ev);
    }
}
