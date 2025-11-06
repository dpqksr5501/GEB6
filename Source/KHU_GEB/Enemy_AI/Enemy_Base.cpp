// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy_AI/Enemy_Base.h"
#include "Engine/Engine.h"

// Sets default values
AEnemy_Base::AEnemy_Base()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEnemy_Base::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemy_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemy_Base::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemy_Base::PerformAttack()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PerformAttack Called"));
	}
}

void AEnemy_Base::PerformSkill()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("PerfromSkill Called"));
	}
}

void AEnemy_Base::PerformSpecialSkill()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("PerfromSpecialSkill Called"));
	}
}