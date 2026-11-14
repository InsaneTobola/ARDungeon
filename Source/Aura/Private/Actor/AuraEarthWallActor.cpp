#include "Actor/AuraEarthWallActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "NavModifierComponent.h"
#include "NavAreas/NavArea_Null.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAuraEarthWallActor::AAuraEarthWallActor()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetReplicateMovement(true);
    NetUpdateFrequency = 30.f;

    WallCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WallCollision"));
    SetRootComponent(WallCollision);

    WallCollision->SetBoxExtent(FVector(50.f, 10.f, 90.f));
    WallCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    WallCollision->SetCollisionObjectType(ECC_WorldDynamic);
    WallCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    WallCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    WallCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    WallCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    WallCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    WallCollision->SetGenerateOverlapEvents(false);
    WallCollision->SetCanEverAffectNavigation(true);

    WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
    WallMesh->SetupAttachment(WallCollision);
    WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WallMesh->SetCanEverAffectNavigation(false);
    WallMesh->SetCastShadow(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (CubeMesh.Succeeded())
    {
        WallMesh->SetStaticMesh(CubeMesh.Object);
    }

    NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
    NavModifier->SetAreaClass(UNavArea_Null::StaticClass());
}

void AAuraEarthWallActor::InitializeWall(
    float InWidthCm,
    float InHeightCm,
    float InThicknessCm,
    float InLifetimeSeconds)
{
    if (!HasAuthority())
    {
        return;
    }

    WallDimensionsCm = FVector(
        FMath::Max(InWidthCm, 1.f),
        FMath::Max(InThicknessCm, 1.f),
        FMath::Max(InHeightCm, 1.f));

    LifetimeSeconds = FMath::Max(InLifetimeSeconds, 0.01f);

    UpdateWallDimensions();
    ForceNetUpdate();
}

void AAuraEarthWallActor::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        SetLifeSpan(LifetimeSeconds);
        ForceNetUpdate();
    }
}

void AAuraEarthWallActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAuraEarthWallActor, WallDimensionsCm);
}

void AAuraEarthWallActor::OnRep_WallDimensions()
{
    UpdateWallDimensions();
}

void AAuraEarthWallActor::UpdateWallDimensions()
{
    if (!WallCollision || !WallMesh)
    {
        return;
    }

    const FVector SafeDimensions(
        FMath::Max(WallDimensionsCm.X, 1.f),
        FMath::Max(WallDimensionsCm.Y, 1.f),
        FMath::Max(WallDimensionsCm.Z, 1.f));

    WallCollision->SetBoxExtent(SafeDimensions * 0.5f, true);

    WallMesh->SetRelativeRotation(FRotator::ZeroRotator);

    if (const UStaticMesh* StaticMesh = WallMesh->GetStaticMesh())
    {
        const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();
        const FVector MeshSize = MeshBounds.BoxExtent * 2.f;

        if (MeshSize.X > KINDA_SMALL_NUMBER &&
            MeshSize.Y > KINDA_SMALL_NUMBER &&
            MeshSize.Z > KINDA_SMALL_NUMBER)
        {
            const FVector MeshScale(
                SafeDimensions.X / MeshSize.X,
                SafeDimensions.Y / MeshSize.Y,
                SafeDimensions.Z / MeshSize.Z);

            WallMesh->SetRelativeScale3D(MeshScale);
            WallMesh->SetRelativeLocation(-MeshBounds.Origin * MeshScale);
        }
    }
}
