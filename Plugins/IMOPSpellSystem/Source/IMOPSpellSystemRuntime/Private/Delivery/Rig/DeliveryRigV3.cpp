#include "Delivery/Rig/DeliveryRigV3.h"

FTransform UDeliveryRigV3::EvalNode(const UDeliveryRigNodeV3* Node, float TimeSeconds)
{
	if (!Node)
	{
		return FTransform::Identity;
	}
	return Node->Evaluate(TimeSeconds);
}

void UDeliveryRigV3::Evaluate(float TimeSeconds, FTransform& OutRootLS, TArray<FTransform>& OutEmittersLS) const
{
	OutRootLS = EvalNode(RootNode, TimeSeconds);

	OutEmittersLS.Reset();
	OutEmittersLS.Reserve(EmitterNodes.Num());

	for (const TObjectPtr<UDeliveryRigNodeV3>& N : EmitterNodes)
	{
		OutEmittersLS.Add(EvalNode(N, TimeSeconds));
	}
}
