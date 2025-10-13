// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"

// Sets default values
AMonsterBase::AMonsterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 1. "���� �Ӹ��� ������ �ʰ� �ϱ�"
	// ���콺(��Ʈ�ѷ�)�� �������� ĳ������ ���� ���� ȸ������ �ʵ��� �����մϴ�.
	bUseControllerRotationYaw = false;

	// 2. "���� ���� ������ �ٶ󺸰� �ϱ�"
	// ĳ������ ������ �������� ���� �ڵ����� ȸ���ϵ��� �����մϴ�.
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// �⺻ �ȱ� �ӵ��� �����մϴ�. (600)
	GetCharacterMovement()->MaxWalkSpeed = 600.f;

}

// Called when the game starts or when spawned
void AMonsterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMonsterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMonsterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// InputActionMove�� ��ȿ�ϴٸ�(�������Ʈ���� �����Ǿ��ٸ�),
		// Triggered(������ ���� ��) ������ �� Move �Լ��� ȣ���ϵ��� ���ε��մϴ�.
		if (InputActionMove)
		{
			EnhancedInputComponent->BindAction(InputActionMove, ETriggerEvent::Triggered, this, &AMonsterBase::Move);
		}
		if (InputActionLook)
		{
			EnhancedInputComponent->BindAction(InputActionLook, ETriggerEvent::Triggered, this, &AMonsterBase::Look);
		}
		if (InputActionShift) // ���� �̸� ����
		{
			// Ű�� ó�� ������ ��(Started) StartShiftAction �Լ���,
			// Ű���� ���� ������ ��(Completed) StopShiftAction �Լ��� ȣ���ϵ��� �����մϴ�.
			EnhancedInputComponent->BindAction(InputActionShift, ETriggerEvent::Started, this, &AMonsterBase::StartShiftAction);
			EnhancedInputComponent->BindAction(InputActionShift, ETriggerEvent::Completed, this, &AMonsterBase::StopShiftAction);
		}
	}

}
void AMonsterBase::SetCharacterState(ECharacterState NewState)
{
	CharacterState = NewState;
}


void AMonsterBase::StartShiftAction()
{
	// �⺻ �ൿ: �ִ� �ȱ� �ӵ��� �޸��� �ӵ�(��: 600)�� �����մϴ�.
	GetCharacterMovement()->MaxWalkSpeed = 900.f;
}

void AMonsterBase::StopShiftAction()
{
	// �⺻ �ൿ: �ִ� �ȱ� �ӵ��� �ٽ� ���� �ȱ� �ӵ�(��: 200)�� �ǵ����ϴ�.
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
}


void AMonsterBase::Look(const FInputActionValue& Value)
{
	// ���콺�� X, Y ������ ���� �����ɴϴ�.
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// X�� �������� ��Ʈ�ѷ��� �¿� ȸ��(Yaw)��, Y�� �������� ���� ȸ��(Pitch)�� �߰��մϴ�.
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}


void AMonsterBase::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 1. ��Ʈ�ѷ�(ī�޶�)�� ���� ȸ�� ���� �����ɴϴ�.
		const FRotator Rotation = Controller->GetControlRotation();
		// 2. ī�޶� ���Ʒ��� ���� ��(Pitch)�� �����ϰ�, ���� ���� ȸ��(Yaw) ���� ����մϴ�.
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 3. �� ���� ȸ�� ���� �������� ���ο� '����'�� '������' ������ ����մϴ�.
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// 4. ���� ���⿡ ���� �������� �߰��մϴ�.
		AddMovementInput(ForwardDirection, MovementVector.Y); // W/S�� ī�޶��� ��/�ڷ�
		AddMovementInput(RightDirection, MovementVector.X);   // A/D�� ī�޶��� ��/���
	}
}