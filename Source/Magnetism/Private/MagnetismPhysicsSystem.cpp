// Fill out your copyright notice in the Description page of Project Settings.


#include "MagnetismPhysicsSystem.h"
#include "MagnetSphere.h"
#include "Kismet/KismetSystemLibrary.h"

void UMagnetismPhysicsSystem::Tick(float DeltaTime)
{
	ApplyAllMagneticForces(DeltaTime);
	UpdateAllLocations(DeltaTime);
	CheckAllSphericalCollisions();
	DrawDebugBounds();
}

void UMagnetismPhysicsSystem::ApplyAllMagneticForces(float DeltaTime) const
{
	for (int i = 0; i < RegisteredMagnets.Num(); i++)
	{
		for (int j = i + 1; j < RegisteredMagnets.Num(); j++)
		{
			ApplyMagneticForce(RegisteredMagnets[i], RegisteredMagnets[j], DeltaTime);
		}
	}
}

void UMagnetismPhysicsSystem::ApplyMagneticForce(AMagnetSphere* MagnetA, AMagnetSphere* MagnetB, float DeltaTime) const
{
	float Distance = FVector::Distance(MagnetA->GetActorLocation(), MagnetB->GetActorLocation());
	float Force = MAGNETISM_CONSTANT * 
		MagnetA->Mass * MagnetA->MagnetStrength * 
		MagnetB->Mass * MagnetB->MagnetStrength / 
		(Distance * Distance);

	FVector AToBNormalized = (MagnetB->GetActorLocation() - MagnetA->GetActorLocation()).GetSafeNormal();
	FVector ForceOnA =
		(MagnetA->IsPositive() != MagnetB->IsPositive()) ?
		AToBNormalized * Force :
		AToBNormalized * Force * -1;

	MagnetA->ApplyForce(ForceOnA * DeltaTime);
	MagnetB->ApplyForce(-ForceOnA * DeltaTime);
}

void UMagnetismPhysicsSystem::UpdateAllLocations(float DeltaTime) const
{
	for (auto* Magnet : RegisteredMagnets)
	{
		Magnet->SetActorLocation(Magnet->GetActorLocation() + Magnet->Velocity * DeltaTime);
		Magnet->Velocity *= DRAG_MULTIPLIER;
	}
}

void UMagnetismPhysicsSystem::CheckAllSphericalCollisions() const
{
	for (int i = 0; i < RegisteredMagnets.Num(); i++)
	{
		for (int j = i + 1; j < RegisteredMagnets.Num(); j++)
		{
			HandleSphericalCollision(RegisteredMagnets[i], RegisteredMagnets[j]);
		}
	}
}

void UMagnetismPhysicsSystem::HandleSphericalCollision(AMagnetSphere* MagnetA, AMagnetSphere* MagnetB) const
{
	if (MagnetA == MagnetB)
	{
		UE_LOG(LogTemp, Warning, TEXT("Don't apply spherical collision to an object for itself"));
		return;
	}

	const float Distance = FVector::Distance(MagnetA->GetActorLocation(), MagnetB->GetActorLocation());
	const float OverlappedDistance = MagnetA->GetSphereRadius() + MagnetB->GetSphereRadius() - Distance;
	if (OverlappedDistance <= 0.0f)
		return;

	const FVector Normal = (MagnetB->GetActorLocation() - MagnetA->GetActorLocation()).GetSafeNormal();

	const FVector RelativeVelocity = MagnetB->Velocity - MagnetA->Velocity;
	const float RelevantSpeed = FMath::Abs(FVector::DotProduct(RelativeVelocity, Normal));

	const float TotalForce = RelevantSpeed * (MagnetA->Mass + MagnetB->Mass) * 0.5f;
	MagnetA->ApplyForce(-Normal * TotalForce);
	MagnetB->ApplyForce(Normal * TotalForce);

	const float MassPercentageA = MagnetA->Mass / (MagnetA->Mass + MagnetB->Mass);
	MagnetA->SetActorLocation(
		MagnetA->GetActorLocation() -
		Normal * OverlappedDistance * (1.0f - MassPercentageA)
	);
	MagnetB->SetActorLocation(
		MagnetB->GetActorLocation() +
		Normal * OverlappedDistance * MassPercentageA
	);
}

void UMagnetismPhysicsSystem::DrawDebugBounds() const
{
	if (RegisteredMagnets.Num() == 0 || IsValid(RegisteredMagnets[0])) return;
	UKismetSystemLibrary::DrawDebugBox(
		RegisteredMagnets[0],
		(UpperBounds + LowerBounds) / 2,
		(UpperBounds - LowerBounds) / 2,
		FLinearColor::Red,
		FRotator::ZeroRotator,
		0.0f,
		50.0f
		);
}

AMagnetSphere* UMagnetismPhysicsSystem::TraceLineForMagnetSpheres(const FVector& RayOrigin, const FVector& RayDirection)
{
	AMagnetSphere* ReturnPtr = nullptr;
	float ClosestHitDistance = MAX_FLT;

	for (auto* Magnet : RegisteredMagnets)
	{
		const FVector RayOriginToSphereCenter = Magnet->GetActorLocation() - RayOrigin;

		//Ray starts inside sphere
		if (RayOriginToSphereCenter.SquaredLength() < Magnet->GetSphereRadius())
			continue;

		const float ClosestPointOnRayDistance = RayOriginToSphereCenter.Dot(RayDirection);

		//Ray points away from sphere
		if (ClosestPointOnRayDistance < 0.0f && RayOriginToSphereCenter.Length() > Magnet->GetSphereRadius())
			continue;

		const float ClosestPointOnSphereDistance = FMath::Sqrt(
			RayOriginToSphereCenter.SquaredLength() - ClosestPointOnRayDistance * ClosestPointOnRayDistance
		);

		//Ray does not intersect
		if (ClosestPointOnSphereDistance > Magnet->GetSphereRadius())
			continue;

		const float IntersectionDistance = ClosestPointOnRayDistance - FMath::Sqrt(
			Magnet->GetSphereRadius() * Magnet->GetSphereRadius() - ClosestPointOnSphereDistance * ClosestPointOnSphereDistance
		);

		//There is a closer intersected object
		if (IntersectionDistance >= ClosestHitDistance)
			continue;

		ReturnPtr = Magnet;
		ClosestHitDistance = IntersectionDistance;
	}

	return ReturnPtr;
}