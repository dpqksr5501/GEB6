// Dragon.cpp

#include "Dragon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h" // SphereTrace�� ���� ����

ADragon::ADragon()
{
	// ������: ������Ʈ �⺻�� ����
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxFlySpeed = MaxFlySpeed; // ����� ����� ���� ���
		MoveComp->BrakingDecelerationFlying = FlyingBrakingDeceleration;
	}
}

void ADragon::BeginPlay()
{
	Super::BeginPlay(); // �θ��� BeginPlay ȣ�� (InputMappingContext �߰� ��)

	// CharacterMovement ������Ʈ�� ã�� ������ ���� (ĳ���� �� ��ȿ�� �˻� �߰�)
	DragonMovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
	// ���� �� �� �� �� �� ���� (������)
	if (DragonMovementComponent)
	{
		DragonMovementComponent->MaxFlySpeed = MaxFlySpeed;
		DragonMovementComponent->BrakingDecelerationFlying = FlyingBrakingDeceleration;
	}
}

void ADragon::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit); // �θ��� Landed ���� ȣ�� (OnLandedEvent ��ȣ �߼�)

	// ���� �Ǵ� �ϰ� �߿� ���� ��Ҵٸ�
	if (bIsFlying && DragonMovementComponent)
	{
		bIsFlying = false; // ���� ���� OFF
		bIsDescending = false; // �ϰ� ���� OFF
		DragonMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
		DragonMovementComponent->GravityScale = 1.f; // �߷� ON
	}
}

void ADragon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // �θ��� Tick ��� ����

	if (DragonMovementComponent)
	{
		// 1. ���� �ϰ� ���� (bIsDescending �÷��װ� True�� ��)
		if (bDescentMovementActive)
		{
			//AddMovementInput(FVector(0.f, 0.f, -5000.f), 1.0f);

			// 2. ���� ���� ���� (SphereTrace)
			FVector Start = GetActorLocation();
			FVector End = Start - FVector(0.f, 0.f, LandingTraceDistance);
			FHitResult HitResult;
			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(this);

			bool bHit = UKismetSystemLibrary::SphereTraceSingle(
				GetWorld(), Start, End, 30.f, // Radius
				UEngineTypes::ConvertToTraceType(ECC_Visibility), false, ActorsToIgnore,
				EDrawDebugTrace::None, HitResult, true);

			// ���� �����ǰ� ���� ���� ���̸� ���� �غ� (Falling ���� ����)
			if (bHit && (DragonMovementComponent->MovementMode == EMovementMode::MOVE_Flying))
			{
				DragonMovementComponent->SetMovementMode(EMovementMode::MOVE_Falling);
			}
		}

	}
}


