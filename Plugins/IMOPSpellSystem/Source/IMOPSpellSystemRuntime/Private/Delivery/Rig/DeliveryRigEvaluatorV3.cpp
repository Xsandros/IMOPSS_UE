#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"
#include "DrawDebugHelpers.h"
#include "Stores/SpellTargetStoreV3.h"

#include "Targeting/TargetingTypesV3.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPDeliveryRigV3, Log, All);

static bool ResolveTargetSetCenter(const USpellTargetStoreV3* Store, FName Key, FVector& OutCenter)
{
	if (!Store || Key == NAME_None) return false;

	const FTargetSetV3* Set = Store->Find(Key);
	if (!Set || Set->Targets.Num() == 0) return false;

	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;

	for (const FTargetRefV3& R : Set->Targets)
	{
		if (AActor* A = Cast<AActor>(R.Actor.Get()))
		{
			Sum += A->GetActorLocation();
			Count++;
		}
	}
	if (Count <= 0) return false;

	OutCenter = Sum / (float)Count;
	return true;
}

static void ApplyLocalOffsetRotation(FDeliveryRigPoseV3& Pose, const FVector& Offset, const FRotator& Rot)
{
	const FRotationMatrix M(Pose.Rotation);
	Pose.Location += M.TransformVector(Offset);

	Pose.Rotation = (Pose.Rotation + Rot).GetNormalized();

	// Keep explicit forward consistent with rotation changes
	if (!Pose.Forward.IsNearlyZero())
	{
		Pose.Forward = FQuat(Rot).RotateVector(Pose.Forward).GetSafeNormal();
	}
	else
	{
		Pose.Forward = Pose.Rotation.Vector();
	}
}


static bool ResolveAttachPose(
	const FSpellExecContextV3& Ctx,
	const FDeliveryContextV3& DeliveryCtx,
	const FDeliveryAttachV3& Attach,
	FDeliveryRigPoseV3& OutPose)
{
	AActor* Caster = Ctx.GetCaster();
	UWorld* World = Ctx.GetWorld();

	switch (Attach.Kind)
	{
	case EDeliveryAttachKindV3::World:
	{
		const FTransform& T = Attach.WorldTransform;
		OutPose.Location = T.GetLocation();
		OutPose.Rotation = T.Rotator();
			OutPose.Forward = OutPose.Rotation.Vector();		
		return true;
	}
	case EDeliveryAttachKindV3::Caster:
	{
		if (!Caster){ return false;}
		OutPose.Location = Caster->GetActorLocation();
		OutPose.Rotation = Caster->GetActorRotation();
		OutPose.Forward = OutPose.Rotation.Vector();	
		return true;
	}
	case EDeliveryAttachKindV3::TargetSet:
	{
		const USpellTargetStoreV3* TS = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get());
		FVector Center;
		if (!ResolveTargetSetCenter(TS, Attach.TargetSetName, Center))
		{
			// fallback to caster
			if (Caster)
			{
				OutPose.Location = Caster->GetActorLocation();
				OutPose.Rotation = Caster->GetActorRotation();
				OutPose.Forward = OutPose.Rotation.Vector();	
				return true;
			}
			return false;
		}
		OutPose.Location = Center;
		OutPose.Rotation = Caster ? Caster->GetActorRotation() : FRotator::ZeroRotator;
		return true;
	}
	case EDeliveryAttachKindV3::ActorRef:
	{
		// Contract-friendly but we don’t have a resolver system yet.
		// For now: "Caster" or empty => caster, otherwise fallback to caster and warn.
		if (Caster && (Attach.ActorRefName == NAME_None || Attach.ActorRefName == "Caster"))
		{
			OutPose.Location = Caster->GetActorLocation();
			OutPose.Rotation = Caster->GetActorRotation();
			OutPose.Forward = OutPose.Rotation.Vector();	
			return true;
		}

		UE_LOG(LogIMOPDeliveryRigV3, Warning,
			TEXT("ResolveAttachPose: ActorRefName=%s not resolvable yet. Falling back to caster."),
			*Attach.ActorRefName.ToString());

		if (Caster)
		{
			OutPose.Location = Caster->GetActorLocation();
			OutPose.Rotation = Caster->GetActorRotation();
			return true;
		}
		return false;
	}
	case EDeliveryAttachKindV3::Socket:
	{
		if (!Caster) return false;

		// Use root component socket if present
		if (USceneComponent* Root = Caster->GetRootComponent())
		{
			if (Attach.SocketName != NAME_None && Root->DoesSocketExist(Attach.SocketName))
			{
				const FTransform T = Root->GetSocketTransform(Attach.SocketName);
				OutPose.Location = T.GetLocation();
				OutPose.Rotation = T.Rotator();
				OutPose.Forward = OutPose.Rotation.Vector();	
				return true;
			}
		}

		// fallback to actor transform
		OutPose.Location = Caster->GetActorLocation();
		OutPose.Rotation = Caster->GetActorRotation();
		return true;
	}
	default:
		break;
	}

	return false;
}

static void AddEmitter(
	const FDeliveryRigPoseV3& Pose,
	int32 SpawnSlot,
	TArray<FDeliveryRigEmitterV3>& OutEmitters)
{
	FDeliveryRigEmitterV3 E;
	E.Pose = Pose;
	E.SpawnSlot = SpawnSlot;
	OutEmitters.Add(E);
}

static void BuildFanEmitters(
	const FDeliveryRigPoseV3& Root,
	int32 Count,
	float ArcDeg,
	const FVector& Axis,
	float Distance,
	int32 BaseSpawnSlot,
	bool bSlotByIndex,
	int32 SlotModulo,
	TArray<FDeliveryRigEmitterV3>& OutEmitters)
{
	OutEmitters.Reset();

	if (Count <= 0 || Distance <= 0.f)
	{
		return;
	}

	const FVector NAxis = Axis.GetSafeNormal();
	const FVector Forward = Root.Rotation.Vector();

	// Centered fan: angles from -Arc/2 to +Arc/2
	const float HalfArc = 0.5f * ArcDeg;

	for (int32 i = 0; i < Count; i++)
	{
		const float Alpha = (Count == 1) ? 0.5f : (float)i / (float)(Count - 1);
		const float AngleDeg = FMath::Lerp(-HalfArc, HalfArc, Alpha);

		const FQuat RotQ(NAxis, FMath::DegreesToRadians(AngleDeg));
		const FVector Dir = RotQ.RotateVector(Forward).GetSafeNormal();

		FDeliveryRigPoseV3 P;
		P.Location = Root.Location + (Dir * Distance);
		P.Rotation = Root.Rotation; // keep root rotation; dir is implicit in location
		

		int32 Slot = BaseSpawnSlot;
		if (bSlotByIndex && SlotModulo > 0)
		{
			Slot = BaseSpawnSlot + (i % SlotModulo);
		}

		AddEmitter(P, Slot, OutEmitters);
	}
}

static void BuildScatterEmitters(
	FRandomStream& Rng,
	const FDeliveryRigPoseV3& Root,
	int32 Count,
	float Radius,
	const FVector& Extents,
	float YawDegrees,
	int32 BaseSpawnSlot,
	bool bSlotByIndex,
	int32 SlotModulo,
	TArray<FDeliveryRigEmitterV3>& OutEmitters)
{
	OutEmitters.Reset();

	if (Count <= 0)
	{
		return;
	}

	const bool bUseSphere = (Radius > 0.f);
	const bool bUseBox = !bUseSphere && !Extents.IsNearlyZero();

	if (!bUseSphere && !bUseBox)
	{
		return;
	}

	for (int32 i = 0; i < Count; i++)
	{
		FVector Local = FVector::ZeroVector;

		if (bUseSphere)
		{
			// Deterministic random point in sphere
			Local = Rng.VRand() * (Rng.FRand() * Radius);
		}
		else
		{
			Local.X = Rng.FRandRange(-Extents.X, Extents.X);
			Local.Y = Rng.FRandRange(-Extents.Y, Extents.Y);
			Local.Z = Rng.FRandRange(-Extents.Z, Extents.Z);
		}

		FDeliveryRigPoseV3 P;
		P.Location = Root.Location + Root.Rotation.RotateVector(Local);
		P.Rotation = Root.Rotation;
		// Radial outward by default (useful for “ring projectiles / rays”)
		P.Forward = Root.Rotation.Vector();

		if (YawDegrees > 0.f)
		{
			const float Y = Rng.FRandRange(-YawDegrees, YawDegrees);
			P.Rotation = (P.Rotation + FRotator(0.f, Y, 0.f)).GetNormalized();
		}

		int32 Slot = BaseSpawnSlot;
		if (bSlotByIndex && SlotModulo > 0)
		{
			Slot = BaseSpawnSlot + (i % SlotModulo);
		}

		AddEmitter(P, Slot, OutEmitters);
	}
}



static void BuildOrbitEmitters(
	const FDeliveryRigPoseV3& Root,
	int32 Count,
	float Radius,
	float PhaseDeg,
	float AngularSpeedDegPerSec,
	float ElapsedSeconds,
	const FVector& Axis,
	int32 BaseSpawnSlot,
	bool bSlotByIndex,
	int32 SlotModulo,
	TArray<FDeliveryRigEmitterV3>& OutEmitters)
{
	if (Count <= 0 || Radius <= 0.f) return;

	const FVector N = Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);

	// Build a basis in the plane perpendicular to N
	FVector X = FVector::CrossProduct(N, FVector::ForwardVector);
	if (X.IsNearlyZero())
	{
		X = FVector::CrossProduct(N, FVector::RightVector);
	}
	X.Normalize();
	const FVector Y = FVector::CrossProduct(N, X).GetSafeNormal();

	const float PhaseRad = FMath::DegreesToRadians(PhaseDeg + (ElapsedSeconds * AngularSpeedDegPerSec));


	OutEmitters.Reset();
	OutEmitters.Reserve(Count);

	for (int32 i = 0; i < Count; i++)
	{
		const float T = (float)i / (float)Count;
		const float A = PhaseRad + T * 2.f * PI;

		const FVector Offset = (FMath::Cos(A) * X + FMath::Sin(A) * Y) * Radius;

		FDeliveryRigEmitterV3 E;
		E.Pose.Location = Root.Location + Offset;
		E.Pose.Rotation = Root.Rotation;

		int32 Slot = BaseSpawnSlot;
		if (bSlotByIndex && SlotModulo > 0)
		{
			Slot = BaseSpawnSlot + (i % SlotModulo);
		}
		E.SpawnSlot = Slot;

		OutEmitters.Add(E);

	}
}

static void ApplyDeterministicJitter(
	FRandomStream& Rng,
	FDeliveryRigPoseV3& Pose,
	float Radius,
	float YawDeg)
{
	if (Radius > 0.f)
	{
		const FVector RandDir = Rng.VRand();
		Pose.Location += RandDir * Radius;
	}
	if (YawDeg > 0.f)
	{
		const float Y = Rng.FRandRange(-YawDeg, YawDeg);
		Pose.Rotation.Yaw = FRotator::NormalizeAxis(Pose.Rotation.Yaw + Y);
	}
}

bool FDeliveryRigEvaluatorV3::Evaluate(
	const FSpellExecContextV3& Ctx,
	const FDeliveryContextV3& DeliveryCtx,
	const FDeliveryRigV3& Rig,
	FDeliveryRigEvalResultV3& OutResult)
{
	const float Now = (Ctx.GetWorld() ? Ctx.GetWorld()->GetTimeSeconds() : 0.f);
	return Evaluate(Ctx, DeliveryCtx, Rig, Now, OutResult);
}

bool FDeliveryRigEvaluatorV3::Evaluate(
	const FSpellExecContextV3& Ctx,
	const FDeliveryContextV3& DeliveryCtx,
	const FDeliveryRigV3& Rig,
	float NowSeconds,
	FDeliveryRigEvalResultV3& OutResult)
{

	OutResult = {};

	const float ElapsedSeconds = (DeliveryCtx.StartTime > 0.f) ? (NowSeconds - DeliveryCtx.StartTime) : 0.f;


	if (Rig.Nodes.Num() == 0)
	{
		// Treat empty rig as "use spec Attach"
		FDeliveryRigPoseV3 RootPose;
		if (!ResolveAttachPose(Ctx, DeliveryCtx, DeliveryCtx.Spec.Attach, RootPose))
		{
			UE_LOG(LogIMOPDeliveryRigV3, Warning, TEXT("Rig Evaluate: empty rig could not resolve Attach. Using origin."));
			RootPose.Location = FVector::ZeroVector;
			RootPose.Rotation = FRotator::ZeroRotator;
		}
		OutResult.Root = RootPose;
		OutResult.Root.Forward = OutResult.Root.Rotation.Vector();

		return true;
	}

	// Deterministic RNG: base on Delivery seed + Rig size
	FRandomStream LocalRng(DeliveryCtx.Seed ^ (Rig.Nodes.Num() * 7919));

	TArray<FDeliveryRigPoseV3> NodePose;
	NodePose.SetNum(Rig.Nodes.Num());

	// Evaluate nodes linearly; Parent must have smaller index or be INDEX_NONE
	for (int32 i = 0; i < Rig.Nodes.Num(); i++)
	{
		const FDeliveryRigNodeV3& N = Rig.Nodes[i];

		FDeliveryRigPoseV3 P;
		if (N.Parent != INDEX_NONE && Rig.Nodes.IsValidIndex(N.Parent))
		{
			P = NodePose[N.Parent];
		}

		switch (N.Kind)
		{
		case EDeliveryRigNodeKindV3::Attach:
		{
			if (!ResolveAttachPose(Ctx, DeliveryCtx, N.Attach, P))
			{
				// fallback to caster attach
				if (!ResolveAttachPose(Ctx, DeliveryCtx, DeliveryCtx.Spec.Attach, P))
				{
					P.Location = FVector::ZeroVector;
					P.Rotation = FRotator::ZeroRotator;
				}
			}
			ApplyLocalOffsetRotation(P, N.LocalOffset, N.LocalRotation);
			break;
		}
		case EDeliveryRigNodeKindV3::Offset:
		{
				ApplyLocalOffsetRotation(P, N.LocalOffset, N.LocalRotation);

				if (N.bAffectEmitters && OutResult.Emitters.Num() > 0)
				{
					for (FDeliveryRigEmitterV3& E : OutResult.Emitters)
					{
						ApplyLocalOffsetRotation(E.Pose, N.LocalOffset, N.LocalRotation);
					}
				}
				break;

		}
		case EDeliveryRigNodeKindV3::AimForward:
		{
			// AimForward just applies local rotation (for now). Future: aim from caster view etc.
			ApplyLocalOffsetRotation(P, N.LocalOffset, N.LocalRotation);
			break;
		}
		case EDeliveryRigNodeKindV3::LookAtTargetSet:
		{
			const USpellTargetStoreV3* TS = Cast<USpellTargetStoreV3>(Ctx.TargetStore.Get());
			const FName Key = (N.LookAtTargetSet != NAME_None) ? N.LookAtTargetSet : N.Attach.TargetSetName;

			FVector Center;
			if (ResolveTargetSetCenter(TS, Key, Center))
			{
				const FVector Dir = (Center - P.Location);
				if (!Dir.IsNearlyZero())
				{
					P.Rotation = Dir.Rotation();
				}
			}
			ApplyLocalOffsetRotation(P, N.LocalOffset, N.LocalRotation);
			break;
		}
		case EDeliveryRigNodeKindV3::RotateOverTime: {
				if (!N.RotationRateDegPerSec.IsZero())
				{
					const FRotator Delta = N.RotationRateDegPerSec * ElapsedSeconds;
					P.Rotation = (P.Rotation + Delta).GetNormalized();
				}
				ApplyLocalOffsetRotation(P, N.LocalOffset, N.LocalRotation);

				if (N.bAffectEmitters && OutResult.Emitters.Num() > 0)
				{
					for (FDeliveryRigEmitterV3& E : OutResult.Emitters)
					{
						if (!N.RotationRateDegPerSec.IsZero())
						{
							const FRotator Delta = N.RotationRateDegPerSec * ElapsedSeconds;
							E.Pose.Rotation = (E.Pose.Rotation + Delta).GetNormalized();
						}
						ApplyLocalOffsetRotation(E.Pose, N.LocalOffset, N.LocalRotation);
					}
				}
				break;
		}
		case EDeliveryRigNodeKindV3::OrbitSampler: 
			{
			// Root stays as-is; emitters will be generated after root is determined.
			ApplyLocalOffsetRotation(P, N.LocalOffset, N.LocalRotation);
			break;
		}
		case EDeliveryRigNodeKindV3::Jitter:
		{
			// Jitter applies later (after orbit) to allow emitter jitter too.
			ApplyLocalOffsetRotation(P, N.LocalOffset, N.LocalRotation);
			break;
		}
		case EDeliveryRigNodeKindV3::FanSampler:
			{
				if (N.FanCount <= 0)
				{
					break;
				}

				TArray<FDeliveryRigEmitterV3> NewEmitters;
				BuildFanEmitters(
					OutResult.Root,
					N.FanCount,
					N.FanArcDegrees,
					N.FanAxis,
					N.FanDistance,
					N.SpawnSlot,
					N.bFanSlotByIndex,
					N.FanSlotModulo,
					NewEmitters);

				if (N.bAppendEmitters && OutResult.Emitters.Num() > 0)
				{
					OutResult.Emitters.Append(NewEmitters);
				}
				else
				{
					OutResult.Emitters = MoveTemp(NewEmitters);
				}
				break;
			}
		default:
			break;
		}

		NodePose[i] = P;
	}

	// Root pose: chosen by RootIndex if valid, else last node.
	const int32 RootIdx = Rig.Nodes.IsValidIndex(Rig.RootIndex) ? Rig.RootIndex : (Rig.Nodes.Num() - 1);
	OutResult.Root = NodePose[RootIdx];

	// Find last orbit sampler (if any) and generate emitters
	for (int32 i = Rig.Nodes.Num() - 1; i >= 0; i--)
	{
		const FDeliveryRigNodeV3& N = Rig.Nodes[i];
		if (N.Kind == EDeliveryRigNodeKindV3::OrbitSampler && N.OrbitCount > 0 && N.OrbitRadius > 0.f)
		{
			TArray<FDeliveryRigEmitterV3> NewEmitters;
			BuildOrbitEmitters(
				OutResult.Root,
				N.OrbitCount,
				N.OrbitRadius,
				N.OrbitPhaseDegrees,
				N.OrbitAngularSpeedDegPerSec,
				ElapsedSeconds,
				N.OrbitAxis,
				N.SpawnSlot,
				N.bOrbitSlotByIndex,
				N.OrbitSlotModulo,
				NewEmitters);

			if (N.bAppendEmitters && OutResult.Emitters.Num() > 0)
			{
				OutResult.Emitters.Append(NewEmitters);
			}
			else
			{
				OutResult.Emitters = MoveTemp(NewEmitters);
			}

			break;
		}
	}

	// Apply jitter nodes (in order) deterministically
	for (int32 i = 0; i < Rig.Nodes.Num(); i++)
	{
		const FDeliveryRigNodeV3& N = Rig.Nodes[i];
		if (N.Kind != EDeliveryRigNodeKindV3::Jitter) continue;
		if (N.JitterRadius <= 0.f && N.JitterYawDegrees <= 0.f) continue;

		FRandomStream R(LocalRng.RandHelper(INT32_MAX) ^ (i * 104729));

		if (N.bApplyJitterToRoot)
		{
			ApplyDeterministicJitter(R, OutResult.Root, N.JitterRadius, N.JitterYawDegrees);
		}
		if (N.bApplyJitterToEmitters)
		{
			for (int32 e = 0; e < OutResult.Emitters.Num(); e++)
			{
				FRandomStream ER(R.GetCurrentSeed() ^ (e * 15485863));
				ApplyDeterministicJitter(ER, OutResult.Emitters[e].Pose, N.JitterRadius, N.JitterYawDegrees);
			}
		}
	}

	// Debug draw (rig root + emitters)
	if (DeliveryCtx.Spec.DebugDraw.bEnable && DeliveryCtx.Spec.DebugDraw.bDrawRig)
	{
		if (UWorld* World = Ctx.GetWorld())
		{
			const float Dur = DeliveryCtx.Spec.DebugDraw.Duration;
			const float Size = DeliveryCtx.Spec.DebugDraw.RigPointSize;

			// Root point
			DrawDebugSphere(World, OutResult.Root.Location, Size, 8, FColor::Cyan, false, Dur, 0, 0.0f);

			// Emitters
			for (int32 i = 0; i < OutResult.Emitters.Num(); i++)
			{
				const FDeliveryRigPoseV3& P = OutResult.Emitters[i].Pose;
				DrawDebugSphere(World, P.Location, Size, 8, FColor::Yellow, false, Dur, 0, 0.0f);

				if (DeliveryCtx.Spec.DebugDraw.bDrawRigAxes)
				{
					const FVector Dir = P.Rotation.Vector();
					DrawDebugDirectionalArrow(World, P.Location, P.Location + Dir * (Size * 3.f), Size * 1.5f, FColor::Yellow, false, Dur, 0, 0.0f);
				}
			}

			if (DeliveryCtx.Spec.DebugDraw.bDrawRigAxes)
			{
				const FVector Dir = OutResult.Root.Rotation.Vector();
				DrawDebugDirectionalArrow(World, OutResult.Root.Location, OutResult.Root.Location + Dir * (Size * 3.f), Size * 1.5f, FColor::Cyan, false, Dur, 0, 0.0f);
			}
		}
	}

	UE_LOG(LogIMOPDeliveryRigV3, VeryVerbose, TEXT("Rig Evaluate: root=%s emitters=%d seed=%d"),
		*OutResult.Root.Location.ToString(), OutResult.Emitters.Num(), DeliveryCtx.Seed);

	return true;
}

const FDeliveryRigPoseV3& FDeliveryRigPoseSelectorV3::SelectPose(const FDeliveryRigEvalResultV3& RigOut, int32 EmitterIndex)
{
	if (EmitterIndex >= 0 && RigOut.Emitters.IsValidIndex(EmitterIndex))
	{
		return RigOut.Emitters[EmitterIndex].Pose;
	}
	return RigOut.Root;
}
