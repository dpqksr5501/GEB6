#include "FlightComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UFlightComponent::UFlightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 매 프레임 트레이스하는 대신 0.1초마다 수행하여 최적화
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UFlightComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
	{
		CachedMovementComp = Owner->GetCharacterMovement();
		if (CachedMovementComp)
		{
			CachedMovementComp->MaxFlySpeed = MaxFlySpeed;
			CachedMovementComp->BrakingDecelerationFlying = FlyingBrakingDeceleration;
		}
	}
}

void UFlightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 강하 중이 아니거나 비행 중이 아니면 트레이스 생략
	if (!bDescentMovementActive || !CachedMovementComp || CachedMovementComp->MovementMode != EMovementMode::MOVE_Flying)
	{
		return;
	}

	FVector Start = GetOwner()->GetActorLocation();
	FVector End = Start - FVector(0.f, 0.f, LandingTraceDistance);
	FHitResult HitResult;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(), Start, End, 30.f,
		UEngineTypes::ConvertToTraceType(ECC_Visibility), false, ActorsToIgnore,
		EDrawDebugTrace::None, HitResult, true);

	if (bHit)
	{
		CachedMovementComp->SetMovementMode(EMovementMode::MOVE_Falling);
	}
}

void UFlightComponent::StartFlight()
{
	bIsFlying = true;
	if (CachedMovementComp)
	{
		CachedMovementComp->SetMovementMode(EMovementMode::MOVE_Flying);
	}
}

void UFlightComponent::StopFlight()
{
	bIsFlying = false;
	bIsDescending = false;
	if (CachedMovementComp)
	{
		CachedMovementComp->SetMovementMode(EMovementMode::MOVE_Falling);
	}
}

void UFlightComponent::OnLanded()
{
	if (bIsFlying && CachedMovementComp)
	{
		bIsFlying = false;
		bIsDescending = false;
		CachedMovementComp->SetMovementMode(EMovementMode::MOVE_Walking);
		CachedMovementComp->GravityScale = 1.f;
	}
}
