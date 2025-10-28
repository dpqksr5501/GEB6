// Dragon.cpp

#include "Dragon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h" // SphereTrace를 위해 포함

ADragon::ADragon()
{
	// 생성자: 컴포넌트 기본값 설정
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxFlySpeed = MaxFlySpeed; // 헤더에 선언된 변수 사용
		MoveComp->BrakingDecelerationFlying = FlyingBrakingDeceleration;
	}
}

void ADragon::BeginPlay()
{
	Super::BeginPlay(); // 부모의 BeginPlay 호출 (InputMappingContext 추가 등)

	// CharacterMovement 컴포넌트를 찾아 변수에 저장 (캐스팅 및 유효성 검사 추가)
	DragonMovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
	// 시작 시 한 번 더 값 설정 (안전성)
	if (DragonMovementComponent)
	{
		DragonMovementComponent->MaxFlySpeed = MaxFlySpeed;
		DragonMovementComponent->BrakingDecelerationFlying = FlyingBrakingDeceleration;
	}
}

void ADragon::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit); // 부모의 Landed 로직 호출 (OnLandedEvent 신호 발송)

	// 비행 또는 하강 중에 땅에 닿았다면
	if (bIsFlying && DragonMovementComponent)
	{
		bIsFlying = false; // 비행 상태 OFF
		bIsDescending = false; // 하강 상태 OFF
		DragonMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
		DragonMovementComponent->GravityScale = 1.f; // 중력 ON
	}
}

void ADragon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // 부모의 Tick 기능 실행

	if (DragonMovementComponent)
	{
		// 1. 지속 하강 로직 (bIsDescending 플래그가 True일 때)
		if (bDescentMovementActive)
		{
			//AddMovementInput(FVector(0.f, 0.f, -5000.f), 1.0f);

			// 2. 착륙 감지 로직 (SphereTrace)
			FVector Start = GetActorLocation();
			FVector End = Start - FVector(0.f, 0.f, LandingTraceDistance);
			FHitResult HitResult;
			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(this);

			bool bHit = UKismetSystemLibrary::SphereTraceSingle(
				GetWorld(), Start, End, 30.f, // Radius
				UEngineTypes::ConvertToTraceType(ECC_Visibility), false, ActorsToIgnore,
				EDrawDebugTrace::None, HitResult, true);

			// 땅이 감지되고 현재 비행 중이면 착륙 준비 (Falling 모드로 변경)
			if (bHit && (DragonMovementComponent->MovementMode == EMovementMode::MOVE_Flying))
			{
				DragonMovementComponent->SetMovementMode(EMovementMode::MOVE_Falling);
			}
		}

	}
}


