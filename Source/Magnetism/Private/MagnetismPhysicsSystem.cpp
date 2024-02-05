// Fill out your copyright notice in the Description page of Project Settings.


#include "MagnetismPhysicsSystem.h"
#include "MagnetSphere.h"
#include "Kismet/KismetSystemLibrary.h"

void UMagnetismPhysicsSystem::Tick(float DeltaTime)
{
	ApplyAllMagneticForces(DeltaTime);
	UpdateAllLocations(DeltaTime);
	CheckAllSphericalCollisions();
	CheckBoundsCollisions();
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
		if (Magnet->Velocity.SquaredLength() < 0.01f)
		{
			Magnet->Velocity = FVector::Zero();
		} 
		else
		{
			if (Magnet->Velocity.SquaredLength() > MAX_VELOCITY * MAX_VELOCITY)
			{
				Magnet->Velocity = Magnet->Velocity.GetSafeNormal() * MAX_VELOCITY;
			}

			Magnet->SetActorLocation(Magnet->GetActorLocation() + Magnet->Velocity * DeltaTime);
			Magnet->Velocity *= DRAG_MULTIPLIER;
		}
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

	//Check if the two objects are actually overlapping.
	const float Distance = FVector::Distance(MagnetA->GetActorLocation(), MagnetB->GetActorLocation());
	const float OverlappedDistance = MagnetA->GetSphereRadius() + MagnetB->GetSphereRadius() - Distance;
	if (OverlappedDistance <= 0.0f)
		return;

	//This is the direction of impact between the two spheres
	const FVector Normal = (MagnetB->GetActorLocation() - MagnetA->GetActorLocation()).GetSafeNormal();

	//The force applied by each magnet is relative to its mass, and its velocity, but only 
	//the part of its velocity that is parallell to the normal direction. 
	const float RelativeSpeedA = FMath::Abs(FVector::DotProduct(MagnetA->Velocity, Normal));
	const float RelativeSpeedB = FMath::Abs(FVector::DotProduct(MagnetB->Velocity, Normal));
	const float TotalForce = RelativeSpeedA * MagnetA->Mass + RelativeSpeedB * MagnetB->Mass;
	
	//The same force is applied to each magnet, regardless of mass; to keep Newton happy.
	MagnetA->ApplyForce(-Normal * TotalForce * DRAG_MULTIPLIER);
	MagnetB->ApplyForce(Normal * TotalForce * DRAG_MULTIPLIER);

	//Finally, we have to counteract the actual overlapping, so we move the two actors apart a bit.
	//The heavier object is moved less.
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

void UMagnetismPhysicsSystem::CheckBoundsCollisions() const
{
	for (auto* Magnet : RegisteredMagnets)
	{
		FVector Location = Magnet->GetActorLocation();
		const float Radius = Magnet->GetSphereRadius();

		if (Location.X - Radius < -BoundsBoxSize)
		{
			Location.X = -BoundsBoxSize + Radius;
			Magnet->Velocity.X *= -1;
		}
		else if (Location.X + Radius > BoundsBoxSize)
		{
			Location.X = BoundsBoxSize - Radius;
			Magnet->Velocity.X *= -1;
		}

		if (Location.Y - Radius < -BoundsBoxSize)
		{
			Location.Y = -BoundsBoxSize + Radius;
			Magnet->Velocity.Y *= -1;
		}
		else if (Location.Y + Radius > BoundsBoxSize)
		{
			Location.Y = BoundsBoxSize - Radius;
			Magnet->Velocity.Y *= -1;
		}

		if (Location.Z - Radius < -BoundsBoxSize)
		{
			Location.Z = -BoundsBoxSize + Radius;
			Magnet->Velocity.Z *= -1;
		}
		else if (Location.Z + Radius > BoundsBoxSize)
		{
			Location.Z = BoundsBoxSize - Radius;
			Magnet->Velocity.Z *= -1;
		}

		Magnet->SetActorLocation(Location);
	}
}

void UMagnetismPhysicsSystem::DrawDebugBounds() const
{
	if (RegisteredMagnets.Num() == 0 || !IsValid(RegisteredMagnets[0])) return;
	UKismetSystemLibrary::DrawDebugBox(
		RegisteredMagnets[0],
		FVector::Zero(),
		FVector::One() * BoundsBoxSize,
		FLinearColor::Red,
		FRotator::ZeroRotator,
		0.0f,
		10.0f
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

FVector UMagnetismPhysicsSystem::RandomSpawnLocation(const float Radius)
{
	FBox Box = FBox(
		-FVector::One() * (BoundsBoxSize - Radius),
		FVector::One() * (BoundsBoxSize - Radius)
	);
	return FMath::RandPointInBox(Box);
}
