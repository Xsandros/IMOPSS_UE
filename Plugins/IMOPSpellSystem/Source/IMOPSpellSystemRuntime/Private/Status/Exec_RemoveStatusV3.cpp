#include "Status/Exec_RemoveStatusV3.h"

#include "Status/SpellPayloadsStatusV3.h"
#include "Status/SpellStatusSubsystemV3.h"
#include "Stores/SpellTargetStoreV3.h"

static UWorld* GetWorldFromExecCtx(const FSpellExecContextV3& Ctx)
{
	return Ctx.WorldContext ? Ctx.WorldContext->GetWorld() : nullptr;
}

static UGameInstance* GetGIFromExecCtx(const FSpellExecContextV3& Ctx)
{
	if (UWorld* W = GetWorldFromExecCtx(Ctx))
	{
		return W->GetGameInstance();
	}
	return nullptr;
}


const UScriptStruct* UExec_RemoveStatusV3::GetPayloadStruct() const
{
	return FPayload_RemoveStatusV3::StaticStruct();
}

void UExec_RemoveStatusV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	const FPayload_RemoveStatusV3* P = static_cast<const FPayload_RemoveStatusV3*>(PayloadData);
	if (!P || !Ctx.TargetStore || !Ctx.WorldContext) return;

	if (!Ctx.bAuthority) return;

	FTargetSetV3 Set;
	if (!Ctx.TargetStore->Get(P->TargetSet, Set)) return;

	UWorld* W = GetWorldFromExecCtx(Ctx);
	USpellStatusSubsystemV3* Status = W ? W->GetSubsystem<USpellStatusSubsystemV3>() : nullptr;
	if (!Status) return;

	for (const FTargetRefV3& R : Set.Targets)
	{
		AActor* Target = R.Actor.Get();
		if (!Target) continue;

		Status->RemoveStatus(Ctx, Target, P->StatusTag);

		// TODO "final": emit Event.Status.Removed
	}
}
