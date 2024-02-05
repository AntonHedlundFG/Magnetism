// Fill out your copyright notice in the Description page of Project Settings.


#include "MagnetSphere.h"
#include "MagnetismPhysicsSystem.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AMagnetSphere::AMagnetSphere()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("MeshComp")));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	SetRootComponent(MeshComp);

}

void AMagnetSphere::SetPositive(bool bIsPositive)
{
	//Update materials if possible
	if (bIsPositive && IsValid(PositiveMaterial))
	{
		MeshComp->SetMaterial(0, PositiveMaterial);
	} else if (!bIsPositive && IsValid(NegativeMaterial))
	{
		MeshComp->SetMaterial(0, NegativeMaterial);
	}

	//save value
	bIsPositiveSaved = bIsPositive;
}

void AMagnetSphere::RandomizeValues()
{
	Mass = FMath::RandRange(1.0f, 5.0f);
	MagnetStrength = FMath::RandRange(1.0f, 10.0f);
	SetPositive(FMath::RandBool());
}

void AMagnetSphere::BeginPlay()
{
	Super::BeginPlay();

	// Spheres are only allowed to be uniformly scaled on each axis.
	const float XScale = GetActorRelativeScale3D().X;
	SetActorRelativeScale3D(FVector::One() * XScale);

	RandomizeValues();

	//Register magnet in physics system
	auto* const MagnetSystem = GetGameInstance()->GetSubsystem<UMagnetismPhysicsSystem>();
	if (IsValid(MagnetSystem))
	{
		MagnetSystem->RegisteredMagnets.Add(this);
	}
}

void AMagnetSphere::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	auto* const MagnetSystem = GetGameInstance()->GetSubsystem<UMagnetismPhysicsSystem>();
	if (IsValid(MagnetSystem))
	{
		MagnetSystem->RegisteredMagnets.RemoveSingleSwap(this);
	}

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AMagnetSphere::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

