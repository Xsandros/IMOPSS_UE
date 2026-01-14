#include "Status/Exec_ApplyStatusV3.h"

#include "Status/SpellPayloadsStatusV3.h"
#include "Status/SpellStatusSubsystemV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Core/SpellExecContextHelpersV3.h"



const UScriptStruct* UExec_ApplyStatusV3::GetPayloadStruct() const
{
	return FPayload_ApplyStatusV3::StaticStruct();
}

void UExec_ApplyStatusV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	const FPayload_ApplyStatusV3* P = static_cast<const FPayload_ApplyStatusV3*>(PayloadData);
	if (!P || !Ctx.TargetStore || !Ctx.WorldContext) return;

	if (!Ctx.bAuthority) return;

	FTargetSetV3 Set;
	if (!Ctx.TargetStore->Get(P->TargetSet, Set)) return;

	UWorld* W = IMOP_GetWorldFromExecCtx(Ctx);
	USpellStatusSubsystemV3* Status = W ? W->GetSubsystem<USpellStatusSubsystemV3>() : nullptr;
	if (!Status) return;

	FEffectContextV3 AppliedCtx;
	AppliedCtx.Source = Ctx.Caster;
	AppliedCtx.Instigator = Ctx.Caster;

	for (const FTargetRefV3& R : Set.Targets)
	{
		AActor* Target = R.Actor.Get();
		if (!Target) continue;

		FActiveStatusV3 Applied;
		Status->ApplyStatus(
			Ctx,
			Target,
			P->Definition,
			AppliedCtx,
			P->DurationOverride,
			P->StacksOverride,
			Applied
		);

		// TODO "final": emit Event.Status.Applied/Rejected etc.
	}
}
