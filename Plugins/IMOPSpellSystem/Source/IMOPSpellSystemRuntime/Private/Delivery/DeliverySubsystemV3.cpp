#include "Delivery/DeliverySubsystemV3.h"

#include "Delivery/Runtime/DeliveryGroupRuntimeV3.h"
#include "Delivery/Rig/DeliveryRigEvaluatorV3.h"
#include "Delivery/Motion/DeliveryMotionTypesV3.h"
#include "Core/SpellGameplayTagsV3.h"

#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "Delivery/Drivers/DeliveryDriver_InstantQueryV3.h"
#include "Delivery/Drivers/DeliveryDriver_FieldV3.h"
#include "Delivery/Drivers/DeliveryDriver_MoverV3.h"
#include "Delivery/Drivers/DeliveryDriver_BeamV3.h"

#include "Events/SpellEventBusSubsystemV3.h"
#include "Runtime/SpellRuntimeV3.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogIMOPDeliveryV3);


static TAutoConsoleVariable<int32> CVarIMOPDeliveryDebugDraw(
	TEXT("imop.Delivery.DebugDraw"),
	0,
	TEXT("0=off, 1=on (draw delivery primitives shapes + ids)"),
	ECVF_Default
);

static TAutoConsoleVariable<float> CVarIMOPDeliveryDebugDrawLife(
	TEXT("imop.Delivery.DebugDrawLife"),
	0.05f,
	TEXT("Lifetime for debug draw shapes (seconds). Keep small to avoid clutter."),
	ECVF_Default
);


// ------------------------------------------------------------
// Local hash helpers (self-contained)
// ------------------------------------------------------------
static uint32 HashU32_Local(uint32 X)
{
	X ^= X >> 16;
	X *= 0x7feb352d;
	X ^= X >> 15;
	X *= 0x846ca68b;
	X ^= X >> 16;
	return X;
}

static uint32 HashCombineLocal(uint32 A, uint32 B)
{
	return HashU32_Local(A ^ (B + 0x9e3779b9u + (A << 6) + (A >> 2)));
}

static uint32 HashGuidStable_Local(const FGuid& G)
{
	uint32 H = 0;
	H = HashCombineLocal(H, HashU32_Local((uint32)G.A));
	H = HashCombineLocal(H, HashU32_Local((uint32)G.B));
	H = HashCombineLocal(H, HashU32_Local((uint32)G.C));
	H = HashCombineLocal(H, HashU32_Local((uint32)G.D));
	return H;
}

static uint32 HashNameStable_Local(const FName& N)
{
	// UE 5.6: GetComparisonIndex() returns FNameEntryId, stable enough for session hashing
	const uint32 A = HashU32_Local((uint32)N.GetComparisonIndex().ToUnstableInt());
	const uint32 B = HashU32_Local((uint32)N.GetNumber());
	return HashCombineLocal(A, B);
}

static uint32 HashCombineFast32(uint32 A, uint32 B)
{
	return A ^ (B + 0x9e3779b9u + (A << 6) + (A >> 2));
}

static FGuid ResolveRuntimeGuidFromCtx(const FSpellExecContextV3& Ctx)
{
	if (const USpellRuntimeV3* R = Cast<USpellRuntimeV3>(Ctx.Runtime.Get()))
	{
		return R->GetRuntimeGuid();
	}
	return FGuid();
}

static FVector RotateVectorByBasis(const FVector& V, const FTransform& Basis)
{
	return Basis.GetRotation().RotateVector(V);
}

static float EaseValue(float T01, EDeliveryEaseV3 Ease)
{
	T01 = FMath::Clamp(T01, 0.f, 1.f);
	switch (Ease)
	{
	case EDeliveryEaseV3::SmoothStep:
		return T01 * T01 * (3.f - 2.f * T01);
	default:
		return T01;
	}
}

static FTransform ApplyPolicyMask(const FTransform& BaseWS, const FTransform& CandidateWS, EDeliveryMotionApplyPolicyV3 Policy, float Weight)
{
	Weight = FMath::Clamp(Weight, 0.f, 1.f);

	// Blend components selectively. We blend towards Candidate based on Weight.
	FVector T = FMath::Lerp(BaseWS.GetTranslation(), CandidateWS.GetTranslation(), Weight);
	FQuat   R = FQuat::Slerp(BaseWS.GetRotation(), CandidateWS.GetRotation(), Weight);
	FVector S = FMath::Lerp(BaseWS.GetScale3D(), CandidateWS.GetScale3D(), Weight);

	switch (Policy)
	{
	case EDeliveryMotionApplyPolicyV3::TranslateOnly:
		return FTransform(BaseWS.GetRotation(), T, BaseWS.GetScale3D());
	case EDeliveryMotionApplyPolicyV3::RotateOnly:
		return FTransform(R, BaseWS.GetTranslation(), BaseWS.GetScale3D());
	case EDeliveryMotionApplyPolicyV3::ScaleOnly:
		return FTransform(BaseWS.GetRotation(), BaseWS.GetTranslation(), S);
	case EDeliveryMotionApplyPolicyV3::TranslateRotate:
		return FTransform(R, T, BaseWS.GetScale3D());
	case EDeliveryMotionApplyPolicyV3::TranslateScale:
		return FTransform(BaseWS.GetRotation(), T, S);
	case EDeliveryMotionApplyPolicyV3::RotateScale:
		return FTransform(R, BaseWS.GetTranslation(), S);
	case EDeliveryMotionApplyPolicyV3::Multiply:
	default:
		return FTransform(R, T, S); // full blend result
	}
}

static float PseudoNoise01(float X)
{
    // deterministic pseudo noise in [0..1]
    return 0.5f + 0.5f * FMath::Sin(X);
}

static FVector PseudoNoiseVec(const float T, const int32 Seed)
{
    const float S = (float)Seed * 0.01337f;
    return FVector(
        PseudoNoise01(T * 3.1f + S + 10.0f) * 2.f - 1.f,
        PseudoNoise01(T * 4.7f + S + 20.0f) * 2.f - 1.f,
        PseudoNoise01(T * 2.3f + S + 30.0f) * 2.f - 1.f
    );
}

static void FixupQueryPolicy(FDeliveryQueryPolicyV3& Q)
{
    // -----------------------------
    // Default FilterMode by QueryMode (only if not explicitly set)
    // -----------------------------
    // If you don't have a "None" state for FilterMode, you can treat ByChannel as "default"
    // and only override when other fields are empty.
    //
    // We'll use: if ByProfile but profile is None -> treat as unset.
    //           if ByObjectType but ObjectTypes empty -> treat as unset.
    //           if ByChannel but TraceChannel is 0 -> treat as unset (rare).

    const bool bProfileUnset = (Q.FilterMode == EDeliveryQueryFilterModeV3::ByProfile) &&
                               (Q.CollisionProfile.Name == NAME_None);

    const bool bObjUnset = (Q.FilterMode == EDeliveryQueryFilterModeV3::ByObjectType) &&
                           (Q.ObjectTypes.Num() == 0);

    const bool bChannelUnset = (Q.FilterMode == EDeliveryQueryFilterModeV3::ByChannel) &&
                               (Q.TraceChannel == ECollisionChannel::ECC_OverlapAll_Deprecated || Q.TraceChannel == 0);

    const bool bFilterEffectivelyUnset = bProfileUnset || bObjUnset || bChannelUnset;

    if (bFilterEffectivelyUnset)
    {
        if (Q.Mode == EDeliveryQueryModeV3::Overlap)
        {
            Q.FilterMode = EDeliveryQueryFilterModeV3::ByObjectType;
        }
        else
        {
            Q.FilterMode = EDeliveryQueryFilterModeV3::ByChannel;
        }
    }

    // -----------------------------
    // Enforce valid combinations
    // -----------------------------
    if (Q.Mode == EDeliveryQueryModeV3::LineTrace)
    {
        if (Q.FilterMode == EDeliveryQueryFilterModeV3::ByObjectType)
        {
            Q.FilterMode = EDeliveryQueryFilterModeV3::ByChannel;
        }
    }

    if (Q.Mode == EDeliveryQueryModeV3::Overlap)
    {
        if (Q.FilterMode == EDeliveryQueryFilterModeV3::ByChannel)
        {
            Q.FilterMode = EDeliveryQueryFilterModeV3::ByObjectType;
        }
    }

    // -----------------------------
    // Default per-filter values (only if missing)
    // -----------------------------
    if (Q.FilterMode == EDeliveryQueryFilterModeV3::ByChannel)
    {
        if (Q.TraceChannel == 0)
        {
            Q.TraceChannel = ECC_Visibility;
        }
    }

    if (Q.FilterMode == EDeliveryQueryFilterModeV3::ByProfile)
    {
        // Leave NAME_None as "use driver defaults" OR set a standard:
        if (Q.CollisionProfile.Name == NAME_None)
        {
            // Safe baseline:
            Q.CollisionProfile.Name = FName(TEXT("BlockAll"));
        }
    }

    if (Q.FilterMode == EDeliveryQueryFilterModeV3::ByObjectType)
    {
        if (Q.ObjectTypes.Num() == 0)
        {
            Q.ObjectTypes.Add(ECC_WorldStatic);
            Q.ObjectTypes.Add(ECC_WorldDynamic);
            Q.ObjectTypes.Add(ECC_Pawn);
        }
    }
}

static TAutoConsoleVariable<int32> CVarIMOPDeliveryValidationLog(
	TEXT("imop.Delivery.ValidationLog"),
	1,
	TEXT("Logs a one-time validation summary when a delivery group starts.\n")
	TEXT("0 = off, 1 = on"),
	ECVF_Default
);

static FString ToString(const FDeliveryShapeV3& S)
{
    switch (S.Kind)
    {
        case EDeliveryShapeV3::Sphere:
            return FString::Printf(TEXT("Sphere r=%.1f"), S.Radius);
        case EDeliveryShapeV3::Box:
            return FString::Printf(TEXT("Box ext=%s"), *S.Extents.ToString());
        case EDeliveryShapeV3::Capsule:
            return FString::Printf(TEXT("Capsule r=%.1f h=%.1f"), S.CapsuleRadius, S.HalfHeight);
        case EDeliveryShapeV3::Ray:
            return FString::Printf(TEXT("Ray len=%.1f"), S.RayLength);
        default:
            return TEXT("UnknownShape");
    }
}

static FString ToString(const FDeliveryQueryPolicyV3& Q)
{
    FString Mode = UEnum::GetValueAsString(Q.Mode);
    FString Filter = UEnum::GetValueAsString(Q.FilterMode);

    switch (Q.FilterMode)
    {
        case EDeliveryQueryFilterModeV3::ByChannel:
            return FString::Printf(TEXT("%s / Channel %s"),
                *Mode,
                *UEnum::GetValueAsString(Q.TraceChannel));

        case EDeliveryQueryFilterModeV3::ByProfile:
            return FString::Printf(TEXT("%s / Profile %s"),
                *Mode,
                *Q.CollisionProfile.Name.ToString());

        case EDeliveryQueryFilterModeV3::ByObjectType:
        {
            FString Obj;
            for (auto C : Q.ObjectTypes)
            {
                if (!Obj.IsEmpty()) Obj += TEXT(",");
                Obj += UEnum::GetValueAsString(C);
            }
            return FString::Printf(TEXT("%s / ObjectType [%s]"), *Mode, *Obj);
        }
    }

    return Mode;
}

static FString ToString(const FDeliveryAnchorRefV3& A)
{
    FString S = UEnum::GetValueAsString(A.Kind);

    if (A.Kind == EDeliveryAnchorRefKindV3::EmitterIndex)
        S += FString::Printf(TEXT("(%d)"), A.EmitterIndex);
    else if (A.Kind == EDeliveryAnchorRefKindV3::PrimitiveId)
        S += FString::Printf(TEXT("(%s)"), *A.TargetPrimitiveId.ToString());

    S += FString::Printf(TEXT(" Follow=%s"),
        *UEnum::GetValueAsString(A.FollowMode));

    return S;
}

static FString ToStringEvents(const FDeliveryPrimitiveSpecV3& P)
{
    TArray<FString> E;

    if (P.Events.bEmitStarted) E.Add(TEXT("Started"));
    if (P.Events.bEmitStopped) E.Add(TEXT("Stopped"));
    if (P.Events.bEmitHit)     E.Add(TEXT("Hit"));

    if (P.Kind == EDeliveryKindV3::Field)
    {
        if (P.FieldEvents.bEmitEnter) E.Add(TEXT("Enter"));
        if (P.FieldEvents.bEmitExit)  E.Add(TEXT("Exit"));
        if (P.FieldEvents.bEmitStay)  E.Add(TEXT("Stay"));
    }

    return FString::Join(E, TEXT(","));
}


static void WarnWeirdKindQueryCombo(const FDeliveryPrimitiveSpecV3& P)
{
	if (P.Kind == EDeliveryKindV3::Field && P.Query.Mode == EDeliveryQueryModeV3::LineTrace)
	{
		UE_LOG(LogIMOPDeliveryV3, Warning, TEXT("Field '%s': QueryMode LineTrace behaves like Beam/InstantQuery; consider Overlap/Sweep."),
			*P.PrimitiveId.ToString());
	}

	if (P.Kind == EDeliveryKindV3::Beam && P.Query.Mode == EDeliveryQueryModeV3::Overlap)
	{
		UE_LOG(LogIMOPDeliveryV3, Warning, TEXT("Beam '%s': QueryMode Overlap is unusual; consider LineTrace/Sweep."),
			*P.PrimitiveId.ToString());
	}
}

static bool IsShapeEffectivelyUnset(const FDeliveryShapeV3& S)
{
    // adapt to your shape struct fields; this is a conservative check
    switch (S.Kind)
    {
        case EDeliveryShapeV3::Sphere:   return S.Radius <= 0.f;
        case EDeliveryShapeV3::Box:      return S.Extents.IsNearlyZero();
        case EDeliveryShapeV3::Capsule:  return (S.CapsuleRadius <= 0.f || S.HalfHeight <= 0.f);
        case EDeliveryShapeV3::Ray:      return S.RayLength <= 0.f;
        default:                         return true;
    }
}

static void ApplyKindDefaultEventPolicy(FDeliveryPrimitiveSpecV3& P)
{
	// Only apply if enabled
	if (!P.Events.bUseKindDefaults)
	{
		return;
	}

	// Baseline: sensible default for all
	P.Events.bEmitStarted = true;
	P.Events.bEmitStopped = true;

	// Tick is usually the #1 spam source -> default depends on kind
	P.Events.bEmitTick = false;

	// Hit default depends on kind
	P.Events.bEmitHit = true;

	// Field/Beam have enter/exit/stay policy
	const bool bHasFieldStyleEvents =
		(P.Kind == EDeliveryKindV3::Field) || (P.Kind == EDeliveryKindV3::Beam);

	if (bHasFieldStyleEvents)
	{
		// “Zone-style” defaults
		P.FieldEvents.bEmitEnter = true;
		P.FieldEvents.bEmitExit  = true;
		P.FieldEvents.bEmitStay  = false;

		// For fields/beams: allow Hit by default (Hit can mean "anything inside")
		P.Events.bEmitHit = true;

		// Tick default: off (you can enable when needed)
		P.Events.bEmitTick = false;
	}
	else
	{
		// Non-field kinds: these flags exist but are irrelevant; no need to touch FieldEvents
	}

	switch (P.Kind)
	{
	case EDeliveryKindV3::Mover:
		// projectiles: tick often useful for motion/debug, but still spammy
		P.Events.bEmitTick = true;
		P.Events.bEmitHit  = true;
		break;

	case EDeliveryKindV3::InstantQuery:
		// one-shot query: tick makes no sense
		P.Events.bEmitTick = false;
		P.Events.bEmitHit  = true;
		break;

	case EDeliveryKindV3::Beam:
		// continuous: you usually want hit/enter/exit, tick optional
		P.Events.bEmitTick = false;
		P.Events.bEmitHit  = true;
		break;

	case EDeliveryKindV3::Field:
		P.Events.bEmitTick = false;
		P.Events.bEmitHit  = true;
		break;

	default:
		break;
	}
}


static void ApplyPrimitiveKindDefaults(FDeliveryPrimitiveSpecV3& P)
{
    // -----------------------------
    // Default QueryMode by Kind (only if "not set")
    // If your enum has no None, we use heuristics:
    // - If Mode is Overlap but Shape is Ray: likely unintended.
    // - Or you can add EDeliveryQueryModeV3::Default / None (better long-term).
    // For now: only change when mode+kind combination is very suspicious.
    // -----------------------------

    // Default Shape by Kind (only if unset)
    if (IsShapeEffectivelyUnset(P.Shape))
    {
        switch (P.Kind)
        {
            case EDeliveryKindV3::InstantQuery:
                P.Shape.Kind = EDeliveryShapeV3::Ray;
                P.Shape.RayLength = 1000.f;
                break;

            case EDeliveryKindV3::Beam:
                P.Shape.Kind = EDeliveryShapeV3::Ray;
                P.Shape.RayLength = 2000.f;
                break;

            case EDeliveryKindV3::Mover:
                P.Shape.Kind = EDeliveryShapeV3::Sphere;
                P.Shape.Radius = 15.f;
                break;

            case EDeliveryKindV3::Field:
            default:
                P.Shape.Kind = EDeliveryShapeV3::Sphere;
                P.Shape.Radius = 150.f;
                break;
        }
    }

    // Nudge QueryMode defaults when clearly mismatched
    // (If you want strict defaults, add a "bUseKindDefaults" for Query too.)
    if (P.Kind == EDeliveryKindV3::Field)
    {
        // If Field + LineTrace and the shape is not Ray, it's weird; prefer Overlap.
        if (P.Query.Mode == EDeliveryQueryModeV3::LineTrace && P.Shape.Kind != EDeliveryShapeV3::Ray)
        {
            P.Query.Mode = EDeliveryQueryModeV3::Overlap;
        }
    }

    if (P.Kind == EDeliveryKindV3::Beam)
    {
        // Beam is almost always line/sweep; Overlap is odd.
        if (P.Query.Mode == EDeliveryQueryModeV3::Overlap)
        {
            P.Query.Mode = EDeliveryQueryModeV3::LineTrace;
        }
    }

    if (P.Kind == EDeliveryKindV3::Mover)
    {
        // Mover with Overlap usually not intended; prefer Sweep.
        if (P.Query.Mode == EDeliveryQueryModeV3::Overlap)
        {
            P.Query.Mode = EDeliveryQueryModeV3::Sweep;
        }
    }

    // After setting Mode, enforce/choose filter defaults
    FixupQueryPolicy(P.Query);
}


static FTransform EvalMotionDelta(
    const FDeliveryMotionSpecV3& M,
    const float Elapsed,
    const int32 Seed,
    const FTransform& AnchorWS,
    const FTransform& BaseWS)
{
    if (!M.bEnabled || !M.Payload.IsValid())
    {
        return FTransform::Identity;
    }

    const UScriptStruct* S = M.Payload.GetScriptStruct();

    // 1) Linear velocity
    if (S == FDeliveryMotion_LinearVelocityV3::StaticStruct())
    {
        const auto& P = M.Payload.Get<FDeliveryMotion_LinearVelocityV3>();

        FVector V = P.Velocity;
        if (P.bVelocityInSpaceBasis)
        {
            if (M.Space == EDeliveryMotionSpaceV3::Local)
                V = RotateVectorByBasis(V, BaseWS);
            else if (M.Space == EDeliveryMotionSpaceV3::Anchor)
                V = RotateVectorByBasis(V, AnchorWS);
            // World: unchanged
        }

        return FTransform(FQuat::Identity, V * Elapsed, FVector::OneVector);
    }

    // 2) Acceleration
    if (S == FDeliveryMotion_AccelerationV3::StaticStruct())
    {
        const auto& P = M.Payload.Get<FDeliveryMotion_AccelerationV3>();

        FVector V0 = P.InitialVelocity;
        FVector A  = P.Acceleration;

        if (M.Space == EDeliveryMotionSpaceV3::Local)
        {
            V0 = RotateVectorByBasis(V0, BaseWS);
            A  = RotateVectorByBasis(A, BaseWS);
        }
        else if (M.Space == EDeliveryMotionSpaceV3::Anchor)
        {
            V0 = RotateVectorByBasis(V0, AnchorWS);
            A  = RotateVectorByBasis(A, AnchorWS);
        }

        const FVector Offset = (V0 * Elapsed) + (0.5f * A * Elapsed * Elapsed);
        return FTransform(FQuat::Identity, Offset, FVector::OneVector);
    }

    // 3) Orbit
    if (S == FDeliveryMotion_OrbitV3::StaticStruct())
    {
        const auto& P = M.Payload.Get<FDeliveryMotion_OrbitV3>();
        const float AngleRad = FMath::DegreesToRadians(P.PhaseDeg + P.AngularSpeedDeg * Elapsed);

        FVector Axis = P.Axis.GetSafeNormal();
        if (Axis.IsNearlyZero()) Axis = FVector(0,0,1);

        // Build an orthonormal basis around Axis deterministically
        FVector U = FVector::CrossProduct(Axis, FVector(1,0,0));
        if (U.IsNearlyZero()) U = FVector::CrossProduct(Axis, FVector(0,1,0));
        U.Normalize();
        FVector V = FVector::CrossProduct(Axis, U).GetSafeNormal();

        FVector LocalOffset = (U * FMath::Cos(AngleRad) + V * FMath::Sin(AngleRad)) * P.Radius;

        if (M.Space == EDeliveryMotionSpaceV3::Local)
            LocalOffset = RotateVectorByBasis(LocalOffset, BaseWS);
        else if (M.Space == EDeliveryMotionSpaceV3::Anchor)
            LocalOffset = RotateVectorByBasis(LocalOffset, AnchorWS);

        return FTransform(FQuat::Identity, LocalOffset, FVector::OneVector);
    }

    // 4) Spin
    if (S == FDeliveryMotion_SpinV3::StaticStruct())
    {
        const auto& P = M.Payload.Get<FDeliveryMotion_SpinV3>();
        const FRotator R = P.AngularVelocityDeg * Elapsed;
        return FTransform(R.Quaternion(), FVector::ZeroVector, FVector::OneVector);
    }

    // 5) Sine offset
    if (S == FDeliveryMotion_SineOffsetV3::StaticStruct())
    {
        const auto& P = M.Payload.Get<FDeliveryMotion_SineOffsetV3>();
        const float W = 2.f * PI * FMath::Max(0.f, P.FrequencyHz);
        const float Sine = FMath::Sin(W * Elapsed + P.Phase);
        FVector Off = P.Amplitude * Sine;

        if (M.Space == EDeliveryMotionSpaceV3::Local)
            Off = RotateVectorByBasis(Off, BaseWS);
        else if (M.Space == EDeliveryMotionSpaceV3::Anchor)
            Off = RotateVectorByBasis(Off, AnchorWS);

        return FTransform(FQuat::Identity, Off, FVector::OneVector);
    }

    // 6) Sine scale
    if (S == FDeliveryMotion_SineScaleV3::StaticStruct())
    {
        const auto& P = M.Payload.Get<FDeliveryMotion_SineScaleV3>();
        const float W = 2.f * PI * FMath::Max(0.f, P.FrequencyHz);
        const float Sine = FMath::Sin(W * Elapsed + P.Phase);
        const float Scale = 1.f + (P.Amplitude * Sine);
        return FTransform(FQuat::Identity, FVector::ZeroVector, FVector(Scale));
    }

    // 7) Noise offset (deterministic)
    if (S == FDeliveryMotion_NoiseOffsetV3::StaticStruct())
    {
        const auto& P = M.Payload.Get<FDeliveryMotion_NoiseOffsetV3>();
        const float W = 2.f * PI * FMath::Max(0.f, P.FrequencyHz);
        const FVector N = PseudoNoiseVec(W * Elapsed, Seed);
        FVector Off = FVector(N.X * P.Amplitude.X, N.Y * P.Amplitude.Y, N.Z * P.Amplitude.Z);

        if (M.Space == EDeliveryMotionSpaceV3::Local)
            Off = RotateVectorByBasis(Off, BaseWS);
        else if (M.Space == EDeliveryMotionSpaceV3::Anchor)
            Off = RotateVectorByBasis(Off, AnchorWS);

        return FTransform(FQuat::Identity, Off, FVector::OneVector);
    }

    // 8) Timed lerp offset
    if (S == FDeliveryMotion_LerpOffsetV3::StaticStruct())
    {
        const auto& P = M.Payload.Get<FDeliveryMotion_LerpOffsetV3>();
        const float D = FMath::Max(KINDA_SMALL_NUMBER, P.Duration);
        const float Alpha = EaseValue(Elapsed / D, P.Ease);
        FVector Off = FMath::Lerp(P.From, P.To, Alpha);

        if (M.Space == EDeliveryMotionSpaceV3::Local)
            Off = RotateVectorByBasis(Off, BaseWS);
        else if (M.Space == EDeliveryMotionSpaceV3::Anchor)
            Off = RotateVectorByBasis(Off, AnchorWS);

        return FTransform(FQuat::Identity, Off, FVector::OneVector);
    }

    return FTransform::Identity;
}


// ------------------------------------------------------------

void UDeliverySubsystemV3::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		if (USpellEventBusSubsystemV3* Bus = World->GetSubsystem<USpellEventBusSubsystemV3>())
		{
			// Avoid overload ambiguity by explicitly selecting UObject overload
			const FGameplayTag RootEventTag = FIMOPSpellGameplayTagsV3::Get().Event; // "Spell.Event"
			SpellEndSub = Bus->Subscribe(Cast<UObject>(this), RootEventTag);

		}
	}
}

void UDeliverySubsystemV3::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (USpellEventBusSubsystemV3* Bus = World->GetSubsystem<USpellEventBusSubsystemV3>())
		{
			Bus->Unsubscribe(SpellEndSub);
		}
	}

	ActiveGroups.Reset();
	NextInstanceByRuntimeAndId.Reset();
	LastRigEvalTimeByHandle.Reset();

	Super::Deinitialize();
}

TStatId UDeliverySubsystemV3::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDeliverySubsystemV3, STATGROUP_Tickables);
}

void UDeliverySubsystemV3::Tick(float DeltaSeconds)
{
	UWorld* W = GetWorld();
	const float Now = W ? W->GetTimeSeconds() : 0.f;

	TArray<FDeliveryHandleV3> Handles;
	GetActiveHandles(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H);
		if (!Group)
		{
			continue;
		}

		const FSpellExecContextV3& Ctx = Group->CtxSnapshot;

		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Time);
		Group->Blackboard.WriteFloat(TEXT("Time.Elapsed"), Now - Group->StartTimeSeconds, EDeliveryBBOwnerV3::Group);

		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Rig);
		EvaluateRigIfNeeded(Group, Now);

		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::DerivedMotion);
		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::PrimitiveMotion);
		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Query);

		TArray<FName> PrimIds;
		PrimIds.Reserve(Group->DriversByPrimitiveId.Num());
		Group->DriversByPrimitiveId.GetKeys(PrimIds);

		// Optional: deterministisch nach PrimitiveIndex sortieren
		PrimIds.Sort([&](const FName& A, const FName& B)
		{
			const FDeliveryContextV3* CA = Group->PrimitiveCtxById.Find(A);
			const FDeliveryContextV3* CB = Group->PrimitiveCtxById.Find(B);
			const int32 IA = CA ? CA->PrimitiveIndex : 0;
			const int32 IB = CB ? CB->PrimitiveIndex : 0;
			if (IA != IB) return IA < IB;
			return A.LexicalLess(B);
		});

		for (const FName& PrimId : PrimIds)
		{
			UDeliveryDriverBaseV3* Driver = Group->DriversByPrimitiveId.FindRef(PrimId);
			if (!Driver || !Driver->IsActive())
			{
				continue;
			}

			Driver->Tick(Ctx, Group, DeltaSeconds);
		}


		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Stop);
		Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Commit);
	}
}

void UDeliverySubsystemV3::OnSpellEvent(const FSpellEventV3& Ev)
{
	if (!Ev.RuntimeGuid.IsValid())
	{
		return;
	}
	
	UE_LOG(LogIMOPDeliveryV3, Display, TEXT("OnSpellEvent: tag=%s prim=%s"),
	*Ev.EventTag.ToString(), *Ev.DeliveryPrimitiveId.ToString());

	// 1) Hard stop for spell end only (precise, no string contains)
	if (Ev.EventTag == FIMOPSpellGameplayTagsV3::Get().Event_Spell_End)
	{
		StopAllForRuntimeGuid(Ev.RuntimeGuid, EDeliveryStopReasonV3::OnEvent);
		return;
	}

	// 2) Per-primitive stop-on-event policies (Model 1)
	// We evaluate only groups of this runtime (optionally narrowed to Ev.DeliveryHandle if policy requires same group).
	TArray<FDeliveryHandleV3> Handles;
	GetActiveHandles(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.RuntimeGuid != Ev.RuntimeGuid)
		{
			continue;
		}

		UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H);
		if (!Group)
		{
			continue;
		}

		// Evaluate policies against the group spec (stable list, safe to stop during loop)
		for (const FDeliveryPrimitiveSpecV3& PSpec : Group->GroupSpec.Primitives)
		{
			if (!PSpec.StopOnEvent.bEnabled)
			{
				continue;
			}

			// Same-group constraint
			if (PSpec.StopOnEvent.bRequireSameGroup)
			{
				// Event must carry a delivery handle to compare; if not present, fail the constraint
				if (!Ev.DeliveryHandle.RuntimeGuid.IsValid() || Ev.DeliveryHandle.DeliveryId == NAME_None)
				{
					continue;
				}
				if (!(Ev.DeliveryHandle == Group->GroupHandle))
				{
					continue;
				}
			}

			// Emitter primitive constraint
			if (PSpec.StopOnEvent.bRequireEmitterPrimitive)
			{
				if (PSpec.StopOnEvent.EmitterPrimitiveId == NAME_None)
				{
					continue;
				}
				if (Ev.DeliveryPrimitiveId != PSpec.StopOnEvent.EmitterPrimitiveId)
				{
					continue;
				}
			}

			// Tag match
			bool bTagMatch = false;
			if (PSpec.StopOnEvent.MatchMode == EDeliveryTagMatchModeV3::Exact)
			{
				bTagMatch = (Ev.EventTag == PSpec.StopOnEvent.EventTag);
			}
			else
			{
				bTagMatch = Ev.EventTag.MatchesTag(PSpec.StopOnEvent.EventTag);
			}

			if (!bTagMatch)
			{
				continue;
			}

			// Required extra tags
			if (!Ev.Tags.HasAll(PSpec.StopOnEvent.RequiredEventTags))
			{
				continue;
			}
			
			UE_LOG(LogIMOPDeliveryV3, Display,
TEXT("StopOnEvent MATCH: targetPrim=%s event=%s emitterPrim=%s group=%s"),
*PSpec.PrimitiveId.ToString(),
*Ev.EventTag.ToString(),
*Ev.DeliveryPrimitiveId.ToString(),
*Group->GroupHandle.DeliveryId.ToString());

			
			// Stop the target primitive (independent!)
			StopPrimitiveInGroup(Group->CtxSnapshot, Group, PSpec.PrimitiveId, EDeliveryStopReasonV3::OnEvent);

			// Group may be auto-removed when last primitive dies (Model 1), so guard pointer usage
			if (!ActiveGroups.Contains(H))
			{
				break;
			}
		}
	}
}

bool UDeliverySubsystemV3::StartDelivery(const FSpellExecContextV3& Ctx, const FDeliverySpecV3& Spec, FDeliveryHandleV3& OutHandle)
{
	UWorld* W = GetWorld();
	if (!W)
	{
		return false;
	}

	if (Spec.DeliveryId == NAME_None)
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Spec.DeliveryId is None"));
		return false;
	}

	if (Spec.Primitives.Num() == 0)
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Spec.Primitives is empty (Composite-first requires >= 1). DeliveryId=%s"),
			*Spec.DeliveryId.ToString());
		return false;
	}
	
	for (const FDeliveryPrimitiveSpecV3& P : Spec.Primitives)
	{
		WarnWeirdKindQueryCombo(P);
	}



	
	
	const FGuid RuntimeGuid = ResolveRuntimeGuidFromCtx(Ctx);
	if (!RuntimeGuid.IsValid())
	{
		UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: RuntimeGuid invalid (Ctx.Runtime missing or not USpellRuntimeV3)"));
		return false;
	}

	const float Now = W->GetTimeSeconds();
	const int32 InstanceIndex = AllocateInstanceIndex(RuntimeGuid, Spec.DeliveryId);

	FDeliveryHandleV3 Handle;
	Handle.RuntimeGuid = RuntimeGuid;
	Handle.DeliveryId = Spec.DeliveryId;
	Handle.InstanceIndex = InstanceIndex;

	UDeliveryGroupRuntimeV3* Group = NewObject<UDeliveryGroupRuntimeV3>(this);
	Group->GroupHandle = Handle;
	Group->GroupSpec = Spec;
	Group->CtxSnapshot = Ctx;
	Group->StartTimeSeconds = Now;
	Group->GroupSeed = ComputeGroupSeed(RuntimeGuid, Spec.DeliveryId, InstanceIndex);
	Group->Blackboard.InitFromSpec(Spec.BlackboardInit, Spec.OwnershipRules);

	for (FDeliveryPrimitiveSpecV3& P : Group->GroupSpec.Primitives)
	{
		ApplyKindDefaultEventPolicy(P);
		ApplyPrimitiveKindDefaults(P);
		FixupQueryPolicy(P.Query);
	}
	
	// First rig evaluation
	Group->Blackboard.BeginPhase(EDeliveryBBPhaseV3::Rig);
	EvaluateRigIfNeeded(Group, Now);
	LastRigEvalTimeByHandle.Add(Handle, Now);

	// Spawn one driver per primitive
	for (int32 i = 0; i < Spec.Primitives.Num(); ++i)
	{
		const FDeliveryPrimitiveSpecV3& PSpec = Spec.Primitives[i];
		if (PSpec.PrimitiveId == NAME_None)
		{
			UE_LOG(LogIMOPDeliveryV3, Error, TEXT("StartDelivery: Primitive[%d] has None PrimitiveId (DeliveryId=%s)"),
				i, *Spec.DeliveryId.ToString());
			continue;
		}

		FDeliveryContextV3 PCtx;
		PCtx.GroupHandle = Handle;
		PCtx.PrimitiveId = PSpec.PrimitiveId;
		PCtx.PrimitiveIndex = i;
		PCtx.Caster = Ctx.GetCaster();
		PCtx.StartTimeSeconds = Now;
		PCtx.Spec = PSpec;

		uint32 Seed = (uint32)Group->GroupSeed;
		Seed = HashCombineFast32(Seed, HashNameStable_Local(PSpec.PrimitiveId));
		Seed = HashCombineFast32(Seed, HashU32_Local((uint32)i));
		PCtx.Seed = (int32)Seed;

		// Pose will be resolved after all contexts exist (supports PrimitiveId anchors)
		PCtx.AnchorPoseWS = Group->RigCache.RootWS;
		const FDeliveryAnchorRefV3& A = PSpec.Anchor;
		const FTransform LocalXf(FQuat(A.LocalRotation), A.LocalOffset, A.LocalScale);
		PCtx.FinalPoseWS = LocalXf * PCtx.AnchorPoseWS;

		Group->PrimitiveCtxById.Add(PSpec.PrimitiveId, PCtx);
	}

	ResolveAllPrimitivePoses(Ctx, Group, Now);

	for (int32 i = 0; i < Spec.Primitives.Num(); ++i)
	{
		const FDeliveryPrimitiveSpecV3& PSpec = Spec.Primitives[i];
		if (PSpec.PrimitiveId == NAME_None) { continue; }

		FDeliveryContextV3* PCtxPtr = Group->PrimitiveCtxById.Find(PSpec.PrimitiveId);
		if (!PCtxPtr) { continue; }

		UDeliveryDriverBaseV3* Driver = CreateDriverForKind(PSpec.Kind);
		if (!Driver) { continue; }

		Driver->GroupHandle = Handle;
		Driver->PrimitiveId = PSpec.PrimitiveId;
		Driver->bActive = true;

		Group->DriversByPrimitiveId.Add(PSpec.PrimitiveId, Driver);
		Driver->Start(Ctx, Group, *PCtxPtr);
	}

	
	ActiveGroups.Add(Handle, Group);
	OutHandle = Handle;
	
	if (CVarIMOPDeliveryValidationLog.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogIMOPDeliveryV3, Display,
			TEXT("[DeliveryValidate] Group=%s Runtime=%s"),
			*Spec.DeliveryId.ToString(),
			*RuntimeGuid.ToString());

		for (const FDeliveryPrimitiveSpecV3& P : Group->GroupSpec.Primitives)
		{
			UE_LOG(LogIMOPDeliveryV3, Display,
				TEXT("  Primitive %s  Kind=%s"),
				*P.PrimitiveId.ToString(),
				*UEnum::GetValueAsString(P.Kind));

			UE_LOG(LogIMOPDeliveryV3, Display,
				TEXT("    Shape=%s"),
				*ToString(P.Shape));

			UE_LOG(LogIMOPDeliveryV3, Display,
				TEXT("    Query=%s"),
				*ToString(P.Query));

			UE_LOG(LogIMOPDeliveryV3, Display,
				TEXT("    Anchor=%s"),
				*ToString(P.Anchor));

			UE_LOG(LogIMOPDeliveryV3, Display,
				TEXT("    Events=%s"),
				*ToStringEvents(P));
		}
	}


	return true;
}

bool UDeliverySubsystemV3::StopDelivery(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason)
{
	return StopGroupInternal(Ctx, Handle, Reason);
}

bool UDeliverySubsystemV3::StopById(const FSpellExecContextV3& Ctx, FName DeliveryId, EDeliveryStopReasonV3 Reason)
{
	if (DeliveryId == NAME_None)
	{
		return false;
	}

	TArray<FDeliveryHandleV3> Handles;
	GetActiveHandles(Handles);

	bool bAny = false;
	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.DeliveryId == DeliveryId)
		{
			bAny |= StopGroupInternal(Ctx, H, Reason);
		}
	}
	return bAny;
}

bool UDeliverySubsystemV3::StopByPrimitiveId(const FSpellExecContextV3& Ctx, FName DeliveryId, FName PrimitiveId, EDeliveryStopReasonV3 Reason)
{
	if (DeliveryId == NAME_None || PrimitiveId == NAME_None)
	{
		return false;
	}

	TArray<FDeliveryHandleV3> Handles;
	GetActiveHandles(Handles);

	bool bAny = false;
	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.DeliveryId != DeliveryId)
		{
			continue;
		}

		UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(H);
		if (!Group)
		{
			continue;
		}

		bAny |= StopPrimitiveInGroup(Ctx, Group, PrimitiveId, Reason);
	}
	return bAny;
}

void UDeliverySubsystemV3::GetActiveHandles(TArray<FDeliveryHandleV3>& Out) const
{
	Out.Reset();

	// Avoid TMap::GetKeys template ambiguity issues by iterating directly.
	for (auto It = ActiveGroups.CreateConstIterator(); It; ++It)
	{
		Out.Add(It.Key());
	}
}

// ------------------------------------------------------------
// Internals
// ------------------------------------------------------------

TObjectPtr<UDeliveryDriverBaseV3> UDeliverySubsystemV3::CreateDriverForKind(EDeliveryKindV3 Kind)
{
	switch (Kind)
	{
	case EDeliveryKindV3::InstantQuery:
		return NewObject<UDeliveryDriver_InstantQueryV3>(this);
	case EDeliveryKindV3::Field:
		return NewObject<UDeliveryDriver_FieldV3>(this);
	case EDeliveryKindV3::Mover:
		return NewObject<UDeliveryDriver_MoverV3>(this);
	case EDeliveryKindV3::Beam:
		return NewObject<UDeliveryDriver_BeamV3>(this);
	default:
		break;
	}
	return nullptr;
}

int32 UDeliverySubsystemV3::AllocateInstanceIndex(const FGuid& RuntimeGuid, FName DeliveryId)
{
	int32& Next = NextInstanceByRuntimeAndId.FindOrAdd(RuntimeGuid).FindOrAdd(DeliveryId);
	Next += 1;
	return Next;
}

int32 UDeliverySubsystemV3::ComputeGroupSeed(const FGuid& RuntimeGuid, FName DeliveryId, int32 InstanceIndex) const
{
	uint32 H = 0;
	H = HashCombineFast32(H, HashGuidStable_Local(RuntimeGuid));
	H = HashCombineFast32(H, HashNameStable_Local(DeliveryId));
	H = HashCombineFast32(H, HashU32_Local((uint32)InstanceIndex));
	return (int32)H;
}

void UDeliverySubsystemV3::EvaluateRigIfNeeded(UDeliveryGroupRuntimeV3* Group, float NowSeconds)
{
	if (!Group)
	{
		return;
	}

	const FDeliverySpecV3& Spec = Group->GroupSpec;

	bool bDoEval = false;
	const float Last = LastRigEvalTimeByHandle.FindRef(Group->GroupHandle);

	switch (Spec.PoseUpdatePolicy)
	{
	case EDeliveryPoseUpdatePolicyV3::EveryTick:
		bDoEval = true;
		break;
	case EDeliveryPoseUpdatePolicyV3::Interval:
	{
		const float Interval = FMath::Max(0.f, Spec.PoseUpdateInterval);
		bDoEval = (Interval <= 0.f) ? true : ((NowSeconds - Last) >= Interval);
		break;
	}
	case EDeliveryPoseUpdatePolicyV3::OnStart:
	default:
		bDoEval = !LastRigEvalTimeByHandle.Contains(Group->GroupHandle);
		break;
	}

	if (!bDoEval)
	{
		return;
	}

	FDeliveryRigEvalResultV3 RigOut;
	UDeliveryRigEvaluatorV3::Evaluate(
		GetWorld(),
		Cast<AActor>(Group->CtxSnapshot.GetCaster()),
		Spec.Attach,
		Spec.Rig,
		NowSeconds - Group->StartTimeSeconds,
		RigOut
	);

	Group->RigCache.RootWS = RigOut.RootWorld;
	Group->RigCache.EmittersWS = RigOut.EmittersWorld;
	Group->RigCache.EmitterNames = RigOut.EmitterNames;

	LastRigEvalTimeByHandle.Add(Group->GroupHandle, NowSeconds);

	ResolveAllPrimitivePoses(Group->CtxSnapshot, Group, NowSeconds);

}

bool UDeliverySubsystemV3::TryResolveAnchorPoseWS(
	const FSpellExecContextV3& /*Ctx*/,
	UDeliveryGroupRuntimeV3* Group,
	const FDeliveryContextV3& PrimitiveCtx,
	FTransform& OutAnchorPoseWS
) const
{
	OutAnchorPoseWS = FTransform::Identity;

	if (!Group)
	{
		return false;
	}

	const FDeliveryAnchorRefV3& A = PrimitiveCtx.Spec.Anchor;

	if (A.Kind == EDeliveryAnchorRefKindV3::Root)
	{
		OutAnchorPoseWS = Group->RigCache.RootWS;
		return true;
	}

	if (A.Kind == EDeliveryAnchorRefKindV3::EmitterIndex)
	{
		if (Group->RigCache.EmittersWS.IsValidIndex(A.EmitterIndex))
		{
			OutAnchorPoseWS = Group->RigCache.EmittersWS[A.EmitterIndex];
			return true;
		}

		OutAnchorPoseWS = Group->RigCache.RootWS;
		return true;
	}

	if (A.Kind == EDeliveryAnchorRefKindV3::PrimitiveId)
	{
		if (A.TargetPrimitiveId == NAME_None)
		{
			return false; // missing reference
		}

		const FDeliveryContextV3* Target = Group->PrimitiveCtxById.Find(A.TargetPrimitiveId);
		if (!Target)
		{
			return false; // not found (stopped or not created)
		}

		OutAnchorPoseWS = Target->FinalPoseWS;
		return true;
	}

	// default fallback
	OutAnchorPoseWS = Group->RigCache.RootWS;
	return true;
}

void UDeliverySubsystemV3::ResolveAllPrimitivePoses(
    const FSpellExecContextV3& Ctx,
    UDeliveryGroupRuntimeV3* Group,
    float NowSeconds
)
{
    if (!Group)
    {
        return;
    }

    // Deterministic order: sort by PrimitiveIndex (stored in ctx)
    TArray<FName> Order;
    Order.Reserve(Group->PrimitiveCtxById.Num());

    for (auto It = Group->PrimitiveCtxById.CreateConstIterator(); It; ++It)
    {
        Order.Add(It.Key());
    }

    Order.Sort([&](const FName& A, const FName& B)
    {
        const FDeliveryContextV3* CA = Group->PrimitiveCtxById.Find(A);
        const FDeliveryContextV3* CB = Group->PrimitiveCtxById.Find(B);
        const int32 IA = CA ? CA->PrimitiveIndex : 0;
        const int32 IB = CB ? CB->PrimitiveIndex : 0;
        if (IA != IB) return IA < IB;
        return A.LexicalLess(B);
    });

    // Collect StopSelf deterministically
    TArray<FName> ToStop;
    ToStop.Reserve(4);

    // N-pass so chains can resolve even if out of order
    const int32 MaxPasses = FMath::Max(1, Order.Num());

    for (int32 Pass = 0; Pass < MaxPasses; ++Pass)
    {
        bool bAnyUpdated = false;

        for (const FName& Pid : Order)
        {
            FDeliveryContextV3* PCtx = Group->PrimitiveCtxById.Find(Pid);
            if (!PCtx)
            {
                continue;
            }

            FTransform AnchorWS;
            const bool bOk = TryResolveAnchorPoseWS(Ctx, Group, *PCtx, AnchorWS);

            if (!bOk)
            {
                switch (PCtx->Spec.OnMissingAnchor)
                {
                case EDeliveryMissingAnchorPolicyV3::Freeze:
                    // Keep previous AnchorPoseWS/FinalPoseWS untouched
                    continue;

                case EDeliveryMissingAnchorPolicyV3::FallbackToRoot:
                    AnchorWS = Group->RigCache.RootWS;
                    break;

                case EDeliveryMissingAnchorPolicyV3::StopSelf:
                    if (!ToStop.Contains(Pid))
                    {
                        ToStop.Add(Pid);
                    }
                    // While waiting to stop, freeze pose (no update)
                    continue;

                default:
                    AnchorWS = Group->RigCache.RootWS;
                    break;
                }
            }

        	const FDeliveryAnchorRefV3& A = PCtx->Spec.Anchor;
        	const FTransform LocalXf(FQuat(A.LocalRotation), A.LocalOffset, A.LocalScale);

        	// Base pose (keep your established multiplication order)
        	const FTransform BaseWS = LocalXf * AnchorWS;

        	// Motion[0] (endformat array, implementation uses only [0])
        	FTransform FinalWS = BaseWS;

        	if (PCtx->Spec.Motions.Num() > 0)
        	{
        		const FDeliveryMotionSpecV3& M0 = PCtx->Spec.Motions[0];
        		if (M0.bEnabled)
        		{
        			const float Elapsed = NowSeconds - Group->StartTimeSeconds;

        			// Evaluate delta
        			const FTransform Delta = EvalMotionDelta(M0, Elapsed, PCtx->Seed, AnchorWS, BaseWS);

        			// Apply depending on space
        			FTransform CandidateWS = BaseWS;
        			switch (M0.Space)
        			{
        			case EDeliveryMotionSpaceV3::Local:
        				CandidateWS = Delta * BaseWS;
        				break;

        			case EDeliveryMotionSpaceV3::Anchor:
        				// Apply delta to anchor first, then local
        				CandidateWS = LocalXf * (Delta * AnchorWS);
        				break;

        			case EDeliveryMotionSpaceV3::World:
        			default:
        				CandidateWS = Delta * BaseWS;
        				break;
        			}

        			FinalWS = ApplyPolicyMask(BaseWS, CandidateWS, M0.ApplyPolicy, M0.Weight);
        		}
        	}

        	PCtx->AnchorPoseWS = AnchorWS;
        	PCtx->FinalPoseWS = FinalWS;


            bAnyUpdated = true;
        }

        // Early out if nothing changed in this pass
        if (!bAnyUpdated)
        {
            break;
        }
    }

    // Execute StopSelf after pose resolution (stable order)
    if (ToStop.Num() > 0)
    {
        ToStop.Sort([&](const FName& A, const FName& B)
        {
            const FDeliveryContextV3* CA = Group->PrimitiveCtxById.Find(A);
            const FDeliveryContextV3* CB = Group->PrimitiveCtxById.Find(B);
            const int32 IA = CA ? CA->PrimitiveIndex : 0;
            const int32 IB = CB ? CB->PrimitiveIndex : 0;
            if (IA != IB) return IA < IB;
            return A.LexicalLess(B);
        });

        for (const FName& Pid : ToStop)
        {
            StopPrimitiveInGroup(Ctx, Group, Pid, EDeliveryStopReasonV3::Failed);
        }
    }
}


void UDeliverySubsystemV3::StopAllForRuntimeGuid(const FGuid& RuntimeGuid, EDeliveryStopReasonV3 Reason)
{
	if (!RuntimeGuid.IsValid())
	{
		return;
	}

	TArray<FDeliveryHandleV3> Handles;
	GetActiveHandles(Handles);

	for (const FDeliveryHandleV3& H : Handles)
	{
		if (H.RuntimeGuid == RuntimeGuid)
		{
			StopGroupInternal(FSpellExecContextV3(), H, Reason);
		}
	}
}

bool UDeliverySubsystemV3::StopPrimitiveInGroup(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, FName PrimitiveId, EDeliveryStopReasonV3 Reason)
{
	if (!Group || PrimitiveId == NAME_None)
	{
		return false;
	}

	UDeliveryDriverBaseV3* Driver = Group->DriversByPrimitiveId.FindRef(PrimitiveId);
	if (Driver && Driver->IsActive())
	{
		Driver->Stop(Ctx, Group, Reason);
		Driver->bActive = false;

		Group->DriversByPrimitiveId.Remove(PrimitiveId);
		Group->PrimitiveCtxById.Remove(PrimitiveId);

		// Modell 1: wenn keine Primitives mehr leben, Group cleanup
		if (Group->DriversByPrimitiveId.Num() == 0)
		{
			const FDeliveryHandleV3 Handle = Group->GroupHandle;
			LastRigEvalTimeByHandle.Remove(Handle);
			ActiveGroups.Remove(Handle);
		}

		return true;

	}

	return false;
}

bool UDeliverySubsystemV3::StopGroupInternal(const FSpellExecContextV3& Ctx, const FDeliveryHandleV3& Handle, EDeliveryStopReasonV3 Reason)
{
	UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(Handle);
	if (!Group)
	{
		return false;
	}

	TArray<FName> Keys;
	Group->DriversByPrimitiveId.GetKeys(Keys);

	for (const FName& Pid : Keys)
	{
		if (UDeliveryDriverBaseV3* Driver = Group->DriversByPrimitiveId.FindRef(Pid))
		{
			if (Driver->IsActive())
			{
				Driver->Stop(Ctx, Group, Reason);
				Driver->bActive = false;
			}
		}
	}

	Group->DriversByPrimitiveId.Reset();
	Group->PrimitiveCtxById.Reset();

	LastRigEvalTimeByHandle.Remove(Handle);
	ActiveGroups.Remove(Handle);

	return true;
}

bool UDeliverySubsystemV3::StopPrimitive(const FSpellExecContextV3& Ctx, const FDeliveryPrimitiveHandleV3& PrimitiveHandle, EDeliveryStopReasonV3 Reason)
{
	if (PrimitiveHandle.PrimitiveId == NAME_None)
	{
		return false;
	}

	UDeliveryGroupRuntimeV3* Group = ActiveGroups.FindRef(PrimitiveHandle.Group);
	if (!Group)
	{
		return false;
	}

	
	
	return StopPrimitiveInGroup(Ctx, Group, PrimitiveHandle.PrimitiveId, Reason);
}
