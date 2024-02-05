// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagnetSphere.generated.h"


UCLASS( hidecategories=(Input, Movement, Collision, Rendering, HLOD, WorldPartition, DataLayers, Replication, Physics, Networking, Actor, LevelInstance, Cooking))
class MAGNETISM_API AMagnetSphere : public AActor
{
	GENERATED_BODY()
	
public:	
	AMagnetSphere();
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category = Magnetism)
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Magnetism, meta = (ClampMin = 1.0f, ClampMax = 5.0f))
	float Mass = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Magnetism, meta = (ClampMin = 1.0f, ClampMax = 10.0f))
	float MagnetStrength = 3.0f;

	UFUNCTION(BlueprintCallable, Category = Magnetism)
	void SetPositive(bool bIsPositive);

	UFUNCTION(BlueprintPure, Category = Magnetism)
	bool IsPositive() { return bIsPositiveSaved; }
	
	// Randomizes values of Mass, MagnetStrength and IsPositive
	UFUNCTION(BlueprintCallable, Category = Magnetism)
	void RandomizeValues();

	void ApplyForce(FVector IncomingForce) { Velocity += IncomingForce / Mass; }
	float GetSphereRadius() const { return GetActorRelativeScale3D().X * 50.0f; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<class UStaticMeshComponent> MeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Magnetism)
	TObjectPtr<class UMaterial> PositiveMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Magnetism)
	TObjectPtr<class UMaterial> NegativeMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Magnetism, meta = (DisplayName="Is Positive"))
	bool bIsPositiveSaved = true;

};
