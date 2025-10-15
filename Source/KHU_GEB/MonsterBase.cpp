// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

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

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}
	
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
		if (InputActionMove)
		{
			EnhancedInputComponent->BindAction(InputActionMove, ETriggerEvent::Triggered, this, &AMonsterBase::Move);
		}
		if (InputActionLook)
		{
			EnhancedInputComponent->BindAction(InputActionLook, ETriggerEvent::Triggered, this, &AMonsterBase::Look);
		}
		if (InputActionShift)
		{
			EnhancedInputComponent->BindAction(InputActionShift, ETriggerEvent::Started, this, &AMonsterBase::StartShiftAction);
			EnhancedInputComponent->BindAction(InputActionShift, ETriggerEvent::Completed, this, &AMonsterBase::StopShiftAction);
		}
		if (InputActionJump)
		{
			EnhancedInputComponent->BindAction(InputActionJump, ETriggerEvent::Started, this, &AMonsterBase::JumpAction_Start);
			EnhancedInputComponent->BindAction(InputActionJump, ETriggerEvent::Triggered, this, &AMonsterBase::JumpAction_Triggered);
			EnhancedInputComponent->BindAction(InputActionJump, ETriggerEvent::Completed, this, &AMonsterBase::JumpAction_Stop);
		}
		if (InputActionCtrl)
		{
			EnhancedInputComponent->BindAction(InputActionCtrl, ETriggerEvent::Started, this, &AMonsterBase::CtrlAction_Start);
		}
		if (InputActionLAttack)
		{
			EnhancedInputComponent->BindAction(InputActionLAttack, ETriggerEvent::Started, this, &AMonsterBase::LAttackAction_Start);
		}
	}
}

void AMonsterBase::SetCharacterState(ECharacterState NewState)
{
	CharacterState = NewState;
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




void AMonsterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	OnLandedEvent(Hit); // �������Ʈ�� OnLandedEvent�� ȣ��
}