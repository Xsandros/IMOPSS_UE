#include "Actions/Exec_EmitEventV3.h"
#include "Actions/SpellPayloadsCoreV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Events/SpellEventV3.h"
#include "Runtime/SpellRuntimeV3.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecEmitEventV3, Log, All);

const UScriptStruct* UExec_EmitEventV3::GetPayloadStruct() const
{
    return FPayload_EmitEventV3::StaticStruct();
}

void UExec_EmitEventV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
    const FPayload_EmitEventV3* Payload = static_cast<const FPayload_EmitEventV3*>(PayloadData);
    if (!Payload || !Payload->EventTag.IsValid())
    {
        UE_LOG(LogIMOPExecEmitEventV3, Warning, TEXT("EmitEvent: invalid payload/tag."));
        return;
    }

    if (!Ctx.EventBus)
    {
        UE_LOG(LogIMOPExecEmitEventV3, Error, TEXT("EmitEvent: missing EventBus."));
        return;
    }

    FSpellEventV3 Ev;
    Ev.EventTag = Payload->EventTag;
    Ev.Caster = Ctx.Caster;
    Ev.Sender = Cast<UObject>(Ctx.Runtime.Get());



    if (UWorld* W = Ctx.WorldContext ? Ctx.WorldContext->GetWorld() : nullptr)
    {
        Ev.FrameNumber = (int32)GFrameCounter;

        Ev.TimeSeconds = (float)W->GetTimeSeconds();
    }

    Ctx.EventBus->Emit(Ev);
}
