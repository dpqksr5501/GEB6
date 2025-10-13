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

	// 1. "몸이 머리를 따라가지 않게 하기"
	// 마우스(컨트롤러)를 움직여도 캐릭터의 몸이 따라 회전하지 않도록 설정합니다.
	bUseControllerRotationYaw = false;

	// 2. "몸이 가는 방향을 바라보게 하기"
	// 캐릭터의 움직임 방향으로 몸이 자동으로 회전하도록 설정합니다.
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// 기본 걷기 속도를 설정합니다. (600)
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
		// InputActionMove가 유효하다면(블루프린트에서 지정되었다면),
		// Triggered(눌리고 있을 때) 상태일 때 Move 함수를 호출하도록 바인딩합니다.
		if (InputActionMove)
		{
			EnhancedInputComponent->BindAction(InputActionMove, ETriggerEvent::Triggered, this, &AMonsterBase::Move);
		}
		if (InputActionLook)
		{
			EnhancedInputComponent->BindAction(InputActionLook, ETriggerEvent::Triggered, this, &AMonsterBase::Look);
		}
		if (InputActionShift) // 변수 이름 변경
		{
			// 키를 처음 눌렀을 때(Started) StartShiftAction 함수를,
			// 키에서 손을 떼었을 때(Completed) StopShiftAction 함수를 호출하도록 연결합니다.
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
	// 기본 행동: 최대 걷기 속도를 달리기 속도(예: 600)로 변경합니다.
	GetCharacterMovement()->MaxWalkSpeed = 900.f;
}

void AMonsterBase::StopShiftAction()
{
	// 기본 행동: 최대 걷기 속도를 다시 원래 걷기 속도(예: 200)로 되돌립니다.
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
}


void AMonsterBase::Look(const FInputActionValue& Value)
{
	// 마우스의 X, Y 움직임 값을 가져옵니다.
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// X축 움직임은 컨트롤러의 좌우 회전(Yaw)에, Y축 움직임은 상하 회전(Pitch)에 추가합니다.
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}


void AMonsterBase::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// 1. 컨트롤러(카메라)의 현재 회전 값을 가져옵니다.
		const FRotator Rotation = Controller->GetControlRotation();
		// 2. 카메라가 위아래를 보는 것(Pitch)은 무시하고, 오직 수평 회전(Yaw) 값만 사용합니다.
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// 3. 이 수평 회전 값을 기준으로 새로운 '앞쪽'과 '오른쪽' 방향을 계산합니다.
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// 4. 계산된 방향에 따라 움직임을 추가합니다.
		AddMovementInput(ForwardDirection, MovementVector.Y); // W/S는 카메라의 앞/뒤로
		AddMovementInput(RightDirection, MovementVector.X);   // A/D는 카메라의 좌/우로
	}
}