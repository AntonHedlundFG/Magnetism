// Fill out your copyright notice in the Description page of Project Settings.


#include "MagnetismBoxDisplay.h"
#include "MagnetismPhysicsSystem.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AMagnetismBoxDisplay::AMagnetismBoxDisplay()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(FName(TEXT("MeshComp")));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	MeshComp->CastShadow = false;
	MeshComp->bAffectDistanceFieldLighting = false;
	MeshComp->bAffectDynamicIndirectLighting = false;
	SetRootComponent(MeshComp);

}

// Called when the game starts or when spawned
void AMagnetismBoxDisplay::BeginPlay()
{
	Super::BeginPlay();

	auto* const MagnetSystem = GetGameInstance()->GetSubsystem<UMagnetismPhysicsSystem>();
	if (IsValid(MagnetSystem))
	{
		MagnetSystem->BoxDisplay = this;
	}
}

// Called every frame
void AMagnetismBoxDisplay::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

