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
#include "HealthComponent.h"
#include "FormManagerComponent.h"
#include "AttackComponent.h"
#include "SkillManagerComponent.h"
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
	AttackManager = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackManager"));
	SkillManager = CreateDefaultSubobject<USkillManagerComponent>(TEXT("SkillManager"));

	static ConstructorHelpers::FObjectFinder<UInputAction> ATTACK(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_Attack.IA_Attack'"));
	if (ATTACK.Object) { AttackAction = ATTACK.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> SKILL(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_Skill.IA_Skill'"));
	if (SKILL.Object) { SkillAction = SKILL.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_BASE(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_Form_Base.IA_Form_Base'"));
	if (FORM_BASE.Object) { FormBase = FORM_BASE.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_RANGE(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_Form_Range.IA_Form_Range'"));
	if (FORM_RANGE.Object) { FormRange = FORM_RANGE.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_SWIFT(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_Form_Swift.IA_Form_Swift'"));
	if (FORM_SWIFT.Object) { FormSwift = FORM_SWIFT.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_GUARD(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_Form_Guard.IA_Form_Guard'"));
	if (FORM_GUARD.Object) { FormGuard = FORM_GUARD.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> FORM_SPECIAL(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_Form_Special.IA_Form_Special'"));
	if (FORM_SPECIAL.Object) { FormSpecial = FORM_SPECIAL.Object; }
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

		// Attack
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, AttackManager, &UAttackComponent::AttackStarted);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, AttackManager, &UAttackComponent::AttackTriggered);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, AttackManager, &UAttackComponent::AttackCompleted);

		// Skill
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Started, this, &AKHU_GEBCharacter::SkillStart);
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Completed, this, &AKHU_GEBCharacter::SkillEnd);

		// 변신
		EnhancedInputComponent->BindAction(FormBase, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToBase);
		EnhancedInputComponent->BindAction(FormRange, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToRange);
		EnhancedInputComponent->BindAction(FormSwift, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToSwift);
		EnhancedInputComponent->BindAction(FormGuard, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToGuard);
		EnhancedInputComponent->BindAction(FormSpecial, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToSpecial);
	}
	else
	{
		UE_LOG(LogKHU_GEB, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AKHU_GEBCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (FormManager)
	{
		FormManager->OnFormChanged.AddDynamic(this, &AKHU_GEBCharacter::OnFormChanged);
		FormManager->InitializeForms();
	}

	OnTakeAnyDamage.AddDynamic(this, &AKHU_GEBCharacter::HandleAnyDamage);
}

void AKHU_GEBCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//UE_LOG(LogTemp, Warning, TEXT("Form = %d"), (int32)FormManager->CurrentForm);
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

void AKHU_GEBCharacter::SkillStart(const FInputActionValue& Value)
{
	if (!SkillManager) return;
	SkillManager->TryActivate(ESkillSlot::Active);
}

void AKHU_GEBCharacter::SkillEnd(const FInputActionValue& Value)
{
	if (!SkillManager) return;
	SkillManager->TryStop(ESkillSlot::Active);
}

void AKHU_GEBCharacter::SwitchToBase(const FInputActionValue& Value)
{
	if (!FormManager) return;
	FormManager->SwitchTo(EFormType::Base);
}

void AKHU_GEBCharacter::SwitchToRange(const FInputActionValue& Value)
{
	if (!FormManager) return;
	FormManager->SwitchTo(EFormType::Range);
}

void AKHU_GEBCharacter::SwitchToSwift(const FInputActionValue& Value)
{
	if (!FormManager) return;
	FormManager->SwitchTo(EFormType::Swift);
}

void AKHU_GEBCharacter::SwitchToGuard(const FInputActionValue& Value)
{
	if (!FormManager) return;
	FormManager->SwitchTo(EFormType::Guard);
}

void AKHU_GEBCharacter::SwitchToSpecial(const FInputActionValue& Value)
{
	if (!FormManager) return;
	FormManager->SwitchTo(EFormType::Special);
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

void AKHU_GEBCharacter::OnFormChanged(EFormType NewForm, const UFormDefinition* Def)
{
	if (AttackManager) { AttackManager->SetForm(Def); }
	if (SkillManager) { SkillManager->EquipFromSkillSet(Def ? Def->SkillSet : nullptr); }
}