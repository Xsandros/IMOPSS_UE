#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliveryTypesV3.h"

struct FDeliveryShapeHelpersV3
{
	// Builds collision shape for overlap/sweep. Returns false if shape kind isn't collision-based (e.g. Ray).
	static bool BuildCollisionShape(const FDeliveryShapeV3& SpecShape, FCollisionShape& OutShape);

	// For ray/line based shapes.
	static float GetRayLength(const FDeliveryShapeV3& SpecShape, float DefaultLen = 1500.f);

	// For debug draw convenience: approximate radius.
	static float GetApproxRadius(const FDeliveryShapeV3& SpecShape);
};
