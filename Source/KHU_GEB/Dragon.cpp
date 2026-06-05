// Dragon.cpp😄

#include "Dragon.h"

ADragon::ADragon()
{
	// Flight logic migrated to FlightComponent
}

void ADragon::BeginPlay()
{
	Super::BeginPlay(); 
}

void ADragon::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
}

void ADragon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


