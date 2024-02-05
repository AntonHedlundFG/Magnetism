// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagnetismBoxDisplay.generated.h"

UCLASS( hidecategories=(Input, Movement, Collision, Rendering, HLOD, WorldPartition, DataLayers, Replication, Physics, Networking, Actor, LevelInstance, Cooking))
class MAGNETISM_API AMagnetismBoxDisplay : public AActor
{
	GENERATED_BODY()
	
public:	
	AMagnetismBoxDisplay();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<class UStaticMeshComponent> MeshComp;
	

};
