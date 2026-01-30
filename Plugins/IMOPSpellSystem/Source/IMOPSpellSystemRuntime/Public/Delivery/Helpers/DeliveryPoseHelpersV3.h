#pragma once

#include "CoreMinimal.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Delivery/DeliveryContextV3.h"

class UDeliveryGroupRuntimeV3;

struct FDeliveryPoseHelpersV3
{
	// Resolves anchor pose WS for a primitive (does not apply LocalXf or Motion).
	static bool ResolveAnchorPoseWS(
		const FSpellExecContextV3& Ctx,
		UDeliveryGroupRuntimeV3* Group,
		FDeliveryContextV3& PCtx,
		FTransform& OutAnchorWS);

	// Applies FollowMode semantics. For World, it is always stable (no follow).
	static void ApplyFollowMode(FDeliveryContextV3& PCtx, FTransform& InOutAnchorWS);

	// Resets freeze state when primitive restarts / spec changes.
	static void ResetFreeze(FDeliveryContextV3& PCtx);
};
