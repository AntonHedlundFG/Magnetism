// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
//#include "Tickable.h"
#include "MagnetismPhysicsSystem.generated.h"

#define MAGNETISM_CONSTANT 100000.0f
#define DRAG_MULTIPLIER 0.99f
#define MAX_VELOCITY 1000.0f

class AMagnetSphere;

/**
 * 
 */
UCLASS(NotBlueprintable)
class MAGNETISM_API UMagnetismPhysicsSystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
	
public:

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = Magnetism)
	TArray<AMagnetSphere*> RegisteredMagnets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Magnetism)
	float BoundsBoxSize = 300.0f;

	UFUNCTION(BlueprintPure, Category = Magnetism)
	AMagnetSphere* TraceLineForMagnetSpheres(const FVector& RayOrigin, const FVector& RayDirection);

	UFUNCTION(BlueprintPure, Category = Magnetism)
	FVector RandomSpawnLocation(const float Radius);

protected:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return Super::GetStatID(); }
	virtual UWorld* GetTickableGameObjectWorld() const { return GetWorld(); }

	void ApplyAllMagneticForces(float DeltaTime) const;
	void ApplyMagneticForce(AMagnetSphere* MagnetA, AMagnetSphere* MagnetB, float DeltaTime) const;

	void UpdateAllLocations(float DeltaTime) const;

	void CheckAllSphericalCollisions() const;
	void HandleSphericalCollision(AMagnetSphere* MagnetA, AMagnetSphere* MagnetB) const;

	void CheckBoundsCollisions() const;

	void DrawDebugBounds() const;
};
