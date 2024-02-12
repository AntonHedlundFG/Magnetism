// Fill out your copyright notice in the Description page of Project Settings.


#include "MagnetismPhysicsSystem.h"
#include "MagnetSphere.h"
#include "MagnetismBoxDisplay.h"
#include "Kismet/KismetSystemLibrary.h"

void UMagnetismPhysicsSystem::Tick(float DeltaTime)
{
	ApplyAllMagneticForces(DeltaTime);
	UpdateVelocitiesAndLocations(DeltaTime);
	CheckAllMagnetToMagnetCollisions();
	RestrainMagnetsWithinBoundsBox();

	if (IsValid(BoxDisplay))
		BoxDisplay->SetActorScale3D(FVector::One() * (BoundsBoxSize / 50));
	else
		DrawBoundsBoxDebug();

}

void UMagnetismPhysicsSystem::ApplyAllMagneticForces(float DeltaTime) const
{
	ParallelFor(RegisteredMagnets.Num(), [&](int32 i) {
		for (int j = i + 1; j < RegisteredMagnets.Num(); j++)
		{
			ApplyMagneticForce(RegisteredMagnets[i], RegisteredMagnets[j], DeltaTime);
		}
		});
}

void UMagnetismPhysicsSystem::ApplyMagneticForce(AMagnetSphere* MagnetA, AMagnetSphere* MagnetB, float DeltaTime) const
{
	const float Distance = FVector::Distance(MagnetA->GetActorLocation(), MagnetB->GetActorLocation());
	
	// Force calculation is similar to gravity; but multiplying each object's mass by it's magnetic strength.
	const float Force = MAGNETISM_CONSTANT * 
		MagnetA->Mass * MagnetA->MagnetStrength * 
		MagnetB->Mass * MagnetB->MagnetStrength / 
		(Distance * Distance);

	//Calculate the force, direction inverts if magnets attract each other
	const FVector AToBNormalized = (MagnetB->GetActorLocation() - MagnetA->GetActorLocation()).GetSafeNormal();
	const FVector ForceOnA =
		(MagnetA->IsPositive() != MagnetB->IsPositive()) ?
		AToBNormalized * Force * DeltaTime: //Attract
		AToBNormalized * Force * DeltaTime * -1; //Repel

	//Apply the same force to the objects, but in opposite directions.
	MagnetA->ApplyForce(ForceOnA);
	MagnetB->ApplyForce(-ForceOnA);
}

void UMagnetismPhysicsSystem::UpdateVelocitiesAndLocations(float DeltaTime) const
{
	for (auto* Magnet : RegisteredMagnets)
	{
		if (Magnet->Velocity.SquaredLength() < 0.01f)
		{
			//For very small velocities, still the object.
			Magnet->Velocity = FVector::Zero();
		} 
		else
		{
			//Restricting maximum velocities to avoid glitchy behaviour in extreme scenarios.
			if (Magnet->Velocity.SquaredLength() > MAX_VELOCITY * MAX_VELOCITY)
			{
				Magnet->Velocity = Magnet->Velocity.GetSafeNormal() * MAX_VELOCITY;
			}

			//Update location and velocity.
			Magnet->SetActorLocation(Magnet->GetActorLocation() + Magnet->Velocity * DeltaTime);
			Magnet->Velocity *= DRAG_MULTIPLIER;
		}
	}
}

void UMagnetismPhysicsSystem::CheckAllMagnetToMagnetCollisions() const
{
	ParallelFor(RegisteredMagnets.Num(), [&](int32 i) {
		for (int j = i + 1; j < RegisteredMagnets.Num(); j++)
		{
			HandleMagnetToMagnetCollision(RegisteredMagnets[i], RegisteredMagnets[j]);
		}
		});
}

void UMagnetismPhysicsSystem::HandleMagnetToMagnetCollision(AMagnetSphere* MagnetA, AMagnetSphere* MagnetB) const
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
	const float RelativeSpeedA = FVector::DotProduct(MagnetA->Velocity, Normal);
	const float RelativeSpeedB = FVector::DotProduct(MagnetB->Velocity, -Normal);
	const float TotalForce = RelativeSpeedA * MagnetA->Mass + RelativeSpeedB * MagnetB->Mass;
	
	//The same force is applied to each magnet, regardless of mass; to keep Newton happy.
	MagnetA->ApplyForce(-Normal * TotalForce * COLLISION_ENERGY_CONSERVED);
	MagnetB->ApplyForce(Normal * TotalForce * COLLISION_ENERGY_CONSERVED);

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

void UMagnetismPhysicsSystem::RestrainMagnetsWithinBoundsBox() const
{
	ParallelFor(RegisteredMagnets.Num(), [&](int32 i) {
		auto* Magnet = RegisteredMagnets[i];
		FVector Location = Magnet->GetActorLocation();
		const float Radius = Magnet->GetSphereRadius();

		//We restrain the magnet's position so that it does not extend outside the bounding box. 
		//If the magnet is outside in one axis, move it inside and flip its velocity in that axis, applying drag.
		if (Location.X - Radius < -BoundsBoxSize)
		{
			Location.X = -BoundsBoxSize + Radius;
			Magnet->Velocity.X *= -COLLISION_ENERGY_CONSERVED;
		}
		else if (Location.X + Radius > BoundsBoxSize)
		{
			Location.X = BoundsBoxSize - Radius;
			Magnet->Velocity.X *= -COLLISION_ENERGY_CONSERVED;
		}

		if (Location.Y - Radius < -BoundsBoxSize)
		{
			Location.Y = -BoundsBoxSize + Radius;
			Magnet->Velocity.Y *= -COLLISION_ENERGY_CONSERVED;
		}
		else if (Location.Y + Radius > BoundsBoxSize)
		{
			Location.Y = BoundsBoxSize - Radius;
			Magnet->Velocity.Y *= -COLLISION_ENERGY_CONSERVED;
		}

		if (Location.Z - Radius < -BoundsBoxSize)
		{
			Location.Z = -BoundsBoxSize + Radius;
			Magnet->Velocity.Z *= -COLLISION_ENERGY_CONSERVED;
		}
		else if (Location.Z + Radius > BoundsBoxSize)
		{
			Location.Z = BoundsBoxSize - Radius;
			Magnet->Velocity.Z *= -COLLISION_ENERGY_CONSERVED;
		}

		//Update magnet with corrected location
		Magnet->SetActorLocation(Location);
		});

}

void UMagnetismPhysicsSystem::DrawBoundsBoxDebug() const
{
	//Cannot draw debug box without world context object. Using first magnet as context.
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

		//Distance from the ray's origin point, to an imagined point along it's direction which is closest to the sphere's center.
		const float ClosestPointOnRayDistance = RayOriginToSphereCenter.Dot(RayDirection);

		//Ray points away from sphere
		if (ClosestPointOnRayDistance < 0.0f && RayOriginToSphereCenter.Length() > Magnet->GetSphereRadius())
			continue;

		//The same imagined point as in the ClosestPointOnRayDistance, but instead it's distance to the sphere's center
		//using the pythagorean theorem.
		const float ClosestPointOnSphereDistance = FMath::Sqrt(
			RayOriginToSphereCenter.SquaredLength() - ClosestPointOnRayDistance * ClosestPointOnRayDistance
		);

		//Ray does not intersect
		if (ClosestPointOnSphereDistance > Magnet->GetSphereRadius())
			continue;

		//Uses pythagorean theorem to find the distance from the imagined closest point to the intersection point,
		//which, when subtracted from the distance from the ray's origin and the imagined closest point,
		//gives the distance from the ray's origin to the intersection point.
		const float IntersectionDistance = ClosestPointOnRayDistance - FMath::Sqrt(
			Magnet->GetSphereRadius() * Magnet->GetSphereRadius() - ClosestPointOnSphereDistance * ClosestPointOnSphereDistance
		);

		//There is a closer intersected object already found
		if (IntersectionDistance >= ClosestHitDistance)
			continue;

		//We've found an intersection, and it's the closest one yet. Update results.
		ReturnPtr = Magnet;
		ClosestHitDistance = IntersectionDistance;
	}

	return ReturnPtr;
}

FVector UMagnetismPhysicsSystem::RandomSpawnLocation(const float Radius)
{
	//Pick a random point within the box defined by BoundsBoxSize, but not close enough to the edge that the sphere
	//would touch the outside.
	FBox Box = FBox(
		-FVector::One() * (BoundsBoxSize - Radius),
		FVector::One() * (BoundsBoxSize - Radius)
	);
	return FMath::RandPointInBox(Box);
}
