#include "Delivery/Helpers/DeliveryShapeHelpersV3.h"

bool FDeliveryShapeHelpersV3::BuildCollisionShape(const FDeliveryShapeV3& S, FCollisionShape& OutShape)
{
	switch (S.Kind)
	{
	case EDeliveryShapeV3::Sphere:
		OutShape = FCollisionShape::MakeSphere(FMath::Max(0.f, S.Radius));
		return true;

	case EDeliveryShapeV3::Box:
		OutShape = FCollisionShape::MakeBox(FVector(
			FMath::Max(0.f, S.Extents.X),
			FMath::Max(0.f, S.Extents.Y),
			FMath::Max(0.f, S.Extents.Z)
		));
		return true;

	case EDeliveryShapeV3::Capsule:
		OutShape = FCollisionShape::MakeCapsule(
			FMath::Max(0.f, S.CapsuleRadius),
			FMath::Max(0.f, S.HalfHeight)
		);
		return true;

	default:
		return false; // Ray etc.
	}
}

float FDeliveryShapeHelpersV3::GetRayLength(const FDeliveryShapeV3& S, float DefaultLen)
{
	if (S.Kind == EDeliveryShapeV3::Ray)
	{
		return (S.RayLength > 0.f) ? S.RayLength : DefaultLen;
	}
	return DefaultLen;
}

float FDeliveryShapeHelpersV3::GetApproxRadius(const FDeliveryShapeV3& S)
{
	switch (S.Kind)
	{
	case EDeliveryShapeV3::Sphere:   return FMath::Max(0.f, S.Radius);
	case EDeliveryShapeV3::Capsule:  return FMath::Max(0.f, S.CapsuleRadius);
	case EDeliveryShapeV3::Box:      return FMath::Max3(FMath::Abs(S.Extents.X), FMath::Abs(S.Extents.Y), FMath::Abs(S.Extents.Z));
	case EDeliveryShapeV3::Ray:      return 0.f;
	default:                         return 0.f;
	}
}
