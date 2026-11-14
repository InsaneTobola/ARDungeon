#include "AbilitySystem/Abilities/AuraEarthWallSpell.h"

#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Actor/AuraEarthWallActor.h"
#include "Character/AuraCharacter.h"
#include "Magic/MagicPatternDefinition.h"
#include "Magic/UAuraMagicComponent.h"
#include "Player/AuraPlayerController.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

UAuraEarthWallSpell::UAuraEarthWallSpell()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

float UAuraEarthWallSpell::GetMaxRangeCm() const
{
    return MaxRangeCm;
}

float UAuraEarthWallSpell::GetWallHeightCm() const
{
    return WallHeightCm;
}

float UAuraEarthWallSpell::GetWallThicknessCm() const
{
    return WallThicknessCm;
}

float UAuraEarthWallSpell::CalculateWallWidthCm(const UAbilitySystemComponent* SourceASC) const
{
    float AreaOfEffect = 0.f;

    if (IsValid(SourceASC))
    {
        AreaOfEffect = SourceASC->GetNumericAttribute(UAuraAttributeSet::GetAreaOfEffectAttribute());
    }

    // Keep the width bounded to the range represented by the approved curve points.
    AreaOfEffect = FMath::Clamp(AreaOfEffect, 0.f, 12.f);

    float WidthCm = BaseWallWidthCm;

    if (IsValid(AreaOfEffectToWidthCurve))
    {
        WidthCm = AreaOfEffectToWidthCurve->GetFloatValue(AreaOfEffect);
    }
    else if (AreaOfEffect <= 4.f)
    {
        WidthCm = FMath::Lerp(BaseWallWidthCm, 110.f, AreaOfEffect / 4.f);
    }
    else if (AreaOfEffect <= 8.f)
    {
        WidthCm = FMath::Lerp(110.f, 150.f, (AreaOfEffect - 4.f) / 4.f);
    }
    else
    {
        WidthCm = FMath::Lerp(150.f, 200.f, (AreaOfEffect - 8.f) / 4.f);
    }

    return FMath::Clamp(WidthCm, MinWallWidthCm, MaxWallWidthCm);
}

void UAuraEarthWallSpell::RejectMagicCast(const FGameplayAbilityActorInfo* ActorInfo) const
{
    AAuraCharacter* Character = ActorInfo
        ? Cast<AAuraCharacter>(ActorInfo->AvatarActor.Get())
        : nullptr;

    if (IsValid(Character) && IsValid(Character->MagicComponent))
    {
        Character->MagicComponent->ClearPendingCast();
    }

    AAuraPlayerController* PC = ActorInfo
        ? Cast<AAuraPlayerController>(ActorInfo->PlayerController.Get())
        : nullptr;

    if (!IsValid(PC) && IsValid(Character))
    {
        PC = Cast<AAuraPlayerController>(Character->GetController());
    }

    if (IsValid(PC))
    {
        PC->ClientMagicTargetRejected();
    }
}

void UAuraEarthWallSpell::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    AActor* Avatar = GetAvatarActorFromActorInfo();
    AAuraCharacter* Character = Cast<AAuraCharacter>(Avatar);
    UAbilitySystemComponent* SourceASC = ActorInfo
        ? ActorInfo->AbilitySystemComponent.Get()
        : nullptr;

    if (!IsValid(Character) ||
        !IsValid(Character->MagicComponent) ||
        !IsValid(SourceASC) ||
        !EarthWallActorClass ||
        !GetWorld())
    {
        RejectMagicCast(ActorInfo);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FName PatternId = NAME_None;
    FVector TargetLocation = FVector::ZeroVector;

    if (!Character->MagicComponent->ConsumePendingCast(PatternId, TargetLocation))
    {
        RejectMagicCast(ActorInfo);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const UMagicPatternDefinition* Pattern =
        Character->MagicComponent->FindPatternDefinition(PatternId);

    if (!IsValid(Pattern) ||
        Pattern->SkillAbilityClass != GetClass() ||
        Pattern->Element != EMagicElement::Earth)
    {
        RejectMagicCast(ActorInfo);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (FVector::Dist2D(Avatar->GetActorLocation(), TargetLocation) > MaxRangeCm)
    {
        RejectMagicCast(ActorInfo);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    const float WallWidth = CalculateWallWidthCm(SourceASC);
    const float WallHeight = FMath::Max(WallHeightCm, 1.f);
    const float WallThickness = FMath::Max(WallThicknessCm, 1.f);

    if (!FMath::IsFinite(WallWidth) || WallWidth <= 0.f)
    {
        RejectMagicCast(ActorInfo);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FVector ToTarget = (TargetLocation - Avatar->GetActorLocation()).GetSafeNormal2D();

    if (ToTarget.IsNearlyZero())
    {
        ToTarget = Avatar->GetActorForwardVector().GetSafeNormal2D();
    }

    // The wall spans perpendicular to the caster-to-target direction.
    const FVector WallDirection(-ToTarget.Y, ToTarget.X, 0.f);
    const FRotator WallRotation(0.f, WallDirection.Rotation().Yaw, 0.f);
    const FVector WallCenter = TargetLocation + FVector(0.f, 0.f, WallHeight * 0.5f);
    const FTransform SpawnTransform(WallRotation, WallCenter);

    AAuraEarthWallActor* Wall = GetWorld()->SpawnActorDeferred<AAuraEarthWallActor>(
        EarthWallActorClass,
        SpawnTransform,
        Avatar,
        Cast<APawn>(Avatar),
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!IsValid(Wall))
    {
        RejectMagicCast(ActorInfo);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Commit only after all server-side validation and deferred actor creation succeeded.
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        Wall->Destroy();
        RejectMagicCast(ActorInfo);
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    Wall->InitializeWall(WallWidth, WallHeight, WallThickness, WallLifetimeSeconds);
    Wall->FinishSpawning(SpawnTransform);

    if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(Character->GetController()))
    {
        PC->ClientMagicTargetAccepted();
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
