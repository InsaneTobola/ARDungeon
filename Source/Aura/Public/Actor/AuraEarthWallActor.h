#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuraEarthWallActor.generated.h"

class UBoxComponent;
class UNavModifierComponent;
class UStaticMeshComponent;

UCLASS()
class AURA_API AAuraEarthWallActor : public AActor
{
    GENERATED_BODY()

public:
    AAuraEarthWallActor();

    void InitializeWall(float InWidthCm, float InHeightCm, float InThicknessCm, float InLifetimeSeconds);

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Earth Wall")
    TObjectPtr<UBoxComponent> WallCollision;

    UPROPERTY(VisibleAnywhere, Category = "Earth Wall")
    TObjectPtr<UStaticMeshComponent> WallMesh;

    UPROPERTY(VisibleAnywhere, Category = "Earth Wall|Navigation")
    TObjectPtr<UNavModifierComponent> NavModifier;

    UPROPERTY(ReplicatedUsing = OnRep_WallDimensions)
    FVector WallDimensionsCm = FVector(100.f, 20.f, 180.f);

    UPROPERTY()
    float LifetimeSeconds = 1.f;

    UFUNCTION()
    void OnRep_WallDimensions();

    void UpdateWallDimensions();
};
