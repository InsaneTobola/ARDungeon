#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AuraEarthWallSpell.generated.h"

class AAuraEarthWallActor;
class UAbilitySystemComponent;
class UCurveFloat;

UCLASS()
class AURA_API UAuraEarthWallSpell : public UAuraGameplayAbility
{
    GENERATED_BODY()

public:
    UAuraEarthWallSpell();

    float GetMaxRangeCm() const;
    float GetWallHeightCm() const;
    float GetWallThicknessCm() const;
    float CalculateWallWidthCm(const UAbilitySystemComponent* SourceASC) const;

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Earth Wall|Targeting", meta = (ClampMin = "1.0"))
    float MaxRangeCm = 1000.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Earth Wall|Dimensions", meta = (ClampMin = "1.0"))
    float WallHeightCm = 180.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Earth Wall|Dimensions", meta = (ClampMin = "1.0"))
    float WallThicknessCm = 20.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Earth Wall|Dimensions", meta = (ClampMin = "1.0"))
    float BaseWallWidthCm = 100.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Earth Wall|Dimensions", meta = (ClampMin = "1.0"))
    float MinWallWidthCm = 100.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Earth Wall|Dimensions", meta = (ClampMin = "1.0"))
    float MaxWallWidthCm = 200.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Earth Wall|Scaling")
    TObjectPtr<UCurveFloat> AreaOfEffectToWidthCurve;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Earth Wall|Lifetime", meta = (ClampMin = "0.01"))
    float WallLifetimeSeconds = 1.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Earth Wall|Actor")
    TSubclassOf<AAuraEarthWallActor> EarthWallActorClass;

private:
    void RejectMagicCast(const FGameplayAbilityActorInfo* ActorInfo) const;
};
