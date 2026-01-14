#include "Effects/Exec_ReadAttributeV3.h"

#include "Effects/SpellPayloadsEffectsV3.h"
#include "Attributes/AttributeSubsystemV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Stores/SpellVariableStoreV3.h"

const UScriptStruct* UExec_ReadAttributeV3::GetPayloadStruct() const
{
	return FPayload_ReadAttributeV3::StaticStruct();
}

void UExec_ReadAttributeV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	const FPayload_ReadAttributeV3* P = static_cast<const FPayload_ReadAttributeV3*>(PayloadData);
	if (!P || !Ctx.TargetStore || !Ctx.VariableStore || !Ctx.WorldContext) return;

	FTargetSetV3 Set;
	if (!Ctx.TargetStore->TryGet(P->TargetSet, Set)) return;

	UGameInstance* GI = Ctx.WorldContext->GetGameInstance();
	if (!GI) return;

	UAttributeSubsystemV3* Attr = GI->GetSubsystem<UAttributeSubsystemV3>();
	if (!Attr || !Attr->GetProvider().GetObject()) return;

	float Acc = 0.f;
	bool bAny = false;

	// Deterministic order: TargetSet array order.
	for (const FTargetRefV3& R : Set.Targets)
	{
		AActor* Target = R.Actor.Get();
		if (!Target) continue;

		float V = 0.f;
		if (Attr->GetAttribute(Target, P->Attribute, V))
		{
			if (!bAny)
			{
				Acc = V;
				bAny = true;
			}
			else
			{
				Acc = P->bSumAcrossTargets ? (Acc + V) : Acc;
			}

			if (!P->bSumAcrossTargets)
			{
				// "first valid" semantics if not summing
				break;
			}
		}
	}

	FSpellValueV3 Out;
	Out.Type = ESpellValueTypeV3::Float;
	Out.Float = bAny ? Acc : 0.f;

	Ctx.VariableStore->SetValue(P->OutVariable, Out);
}
