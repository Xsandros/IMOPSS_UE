#include "Delivery/Helpers/DeliveryPoseHelpersV3.h"
#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"

bool FDeliveryPoseHelpersV3::ResolveAnchorPoseWS(
	const FSpellExecContextV3& Ctx,
	UDeliveryGroupRuntimeV3* Group,
	FDeliveryContextV3& PCtx,
	FTransform& OutAnchorWS)
{
	if (!Group) return false;

	// IMPORTANT: World is absolute and always resolves.
	if (PCtx.Spec.Anchor.Kind == EDeliveryAnchorRefKindV3::World)
	{
		OutAnchorWS = FTransform::Identity;
		return true;
	}

	// Delegate to your existing resolver (so behavior stays consistent),
	// but ensure it cannot accidentally "fake world" again.
	// If your existing function is in DeliverySubsystemV3.cpp as TryResolveAnchorPoseWS,
	// you can either:
	//  A) move that function into a shared helper too, or
	//  B) keep it and call it from the subsystem (recommended for now).

	return false; // caller uses existing TryResolveAnchorPoseWS for non-world
}

void FDeliveryPoseHelpersV3::ApplyFollowMode(FDeliveryContextV3& PCtx, FTransform& InOutAnchorWS)
{
	// World never "follows" anything. FollowMode is irrelevant.
	if (PCtx.Spec.Anchor.Kind == EDeliveryAnchorRefKindV3::World)
	{
		PCtx.bAnchorFrozenWS = false;
		PCtx.FrozenAnchorWS = FTransform::Identity;
		return;
	}

	if (PCtx.Spec.Anchor.FollowMode == EDeliveryAnchorFollowModeV3::FreezeOnPlace)
	{
		if (!PCtx.bAnchorFrozenWS)
		{
			PCtx.bAnchorFrozenWS = true;
			PCtx.FrozenAnchorWS = InOutAnchorWS;
		}
		else
		{
			InOutAnchorWS = PCtx.FrozenAnchorWS;
		}
	}
	else
	{
		PCtx.bAnchorFrozenWS = false;
		PCtx.FrozenAnchorWS = FTransform::Identity;
	}
}

void FDeliveryPoseHelpersV3::ResetFreeze(FDeliveryContextV3& PCtx)
{
	PCtx.bAnchorFrozenWS = false;
	PCtx.FrozenAnchorWS = FTransform::Identity;
}
