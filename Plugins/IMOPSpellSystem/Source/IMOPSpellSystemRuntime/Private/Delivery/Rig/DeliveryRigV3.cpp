#include "Delivery/Rig/DeliveryRigV3.h"

#include "Delivery/Rig/DeliveryRigNodeV3.h"

FTransform UDeliveryRigV3::EvalNode(const UDeliveryRigNodeV3* Node, float TimeSeconds)
{
	return Node ? Node->Evaluate(TimeSeconds) : FTransform::Identity;
}

void UDeliveryRigV3::Evaluate(float TimeSeconds, FTransform& OutRootLS, TArray<FTransform>& OutEmittersLS) const
{
	OutRootLS = EvalNode(RootNode, TimeSeconds);

	OutEmittersLS.Reset();
	OutEmittersLS.Reserve(EmitterNodes.Num());

	for (const TObjectPtr<UDeliveryRigNodeV3>& Node : EmitterNodes)
	{
		OutEmittersLS.Add(EvalNode(Node, TimeSeconds));
	}
}
