// Copyright Epic Games, Inc. All Rights Reserved.

#include "KHU_GEBCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "FormManagerComponent.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "KHU_GEB.h"


AKHU_GEBCharacter::AKHU_GEBCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	FormManager = CreateDefaultSubobject<UFormManagerComponent>(TEXT("FormManager"));

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_BASE(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_BaseForm.IA_BaseForm'"));
	if (FORM_BASE.Object) { BaseForm = FORM_BASE.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_RANGE(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_RangeForm.IA_RangeForm'"));
	if (FORM_RANGE.Object) { RangeForm = FORM_RANGE.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_SPEED(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_SpeedForm.IA_SpeedForm'"));
	if (FORM_SPEED.Object) { SpeedForm = FORM_SPEED.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_DEFENSE(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_DefenseForm.IA_DefenseForm'"));
	if (FORM_DEFENSE.Object) { DefenseForm = FORM_DEFENSE.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_DEBUFF(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_DebuffForm.IA_DebuffForm'"));
	if (FORM_DEBUFF.Object) { DebuffForm = FORM_DEBUFF.Object; }
}

void AKHU_GEBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::Look);

		// 변신
		EnhancedInputComponent->BindAction(BaseForm, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToBase);
		EnhancedInputComponent->BindAction(RangeForm, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToRange);
		EnhancedInputComponent->BindAction(SpeedForm, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToSpeed);
		EnhancedInputComponent->BindAction(DefenseForm, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToDefense);
		EnhancedInputComponent->BindAction(DebuffForm, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToDebuff);
	}
	else
	{
		UE_LOG(LogKHU_GEB, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AKHU_GEBCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (FormManager) { FormManager->InitializeForms(FormManager->CurrentForm); }

	OnTakeAnyDamage.AddDynamic(this, &AKHU_GEBCharacter::HandleAnyDamage);
}

void AKHU_GEBCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UE_LOG(LogTemp, Warning, TEXT("Form = %d"), (int32)FormManager->CurrentForm);
}

float AKHU_GEBCharacter::GetHealth() const
{
	return HealthComp ? HealthComp->Health : 0.f;
}

void AKHU_GEBCharacter::Heal(float Amount)
{
	if (HealthComp)
	{
		HealthComp->AddHealth(Amount);
	}
}

void  AKHU_GEBCharacter::HandleAnyDamage(AActor* DamagedActor, float Damage,
	const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (HealthComp && Damage > 0.f)
	{
		HealthComp->ReduceHealth(Damage);
	}
}

void AKHU_GEBCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AKHU_GEBCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AKHU_GEBCharacter::SwitchToBase(const FInputActionValue& Value)
{
	if (!FormManager) return;
	if (FormManager->CurrentForm == EPlayerForm::Base) return;	
	FormManager->SwitchTo(EPlayerForm::Base);
}

void AKHU_GEBCharacter::SwitchToRange(const FInputActionValue& Value)
{
	if (!FormManager) return;
	if (FormManager->CurrentForm == EPlayerForm::Range) return;
	FormManager->SwitchTo(EPlayerForm::Range);
}

void AKHU_GEBCharacter::SwitchToSpeed(const FInputActionValue& Value)
{
	if (!FormManager) return;
	if (FormManager->CurrentForm == EPlayerForm::Speed) return;
	FormManager->SwitchTo(EPlayerForm::Speed);
}

void AKHU_GEBCharacter::SwitchToDefense(const FInputActionValue& Value)
{
	if (!FormManager) return;
	if (FormManager->CurrentForm == EPlayerForm::Defense) return;
	FormManager->SwitchTo(EPlayerForm::Defense);
}

void AKHU_GEBCharacter::SwitchToDebuff(const FInputActionValue& Value)
{
	if (!FormManager) return;
	if (FormManager->CurrentForm == EPlayerForm::Debuff) return;
	FormManager->SwitchTo(EPlayerForm::Debuff);
}

void AKHU_GEBCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AKHU_GEBCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AKHU_GEBCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AKHU_GEBCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
