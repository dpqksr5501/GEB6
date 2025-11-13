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
#include "FormDefinition.h"

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

	static ConstructorHelpers::FObjectFinder<UInputAction> SPRINT_ACTION(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_Shift.IA_Shift'"));
	if (SPRINT_ACTION.Object) {	SprintAction = SPRINT_ACTION.Object; }

	CurrentFormBaseSpeed = 600.f; // 기본값 (DA_Base의 값과 일치시키는 것이 좋음)
	bIsSprinting = false;


	//인터페이스용 변수 2개를 초기화합니다.
	CurrentPlayerState = ECharacterState::Idle;
	bPlayerWantsToJump = false;
}

void AKHU_GEBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);


		//추가 코드
		//점프 바인딩을 수정
		//ACharacter::Jump 대신 만든 StartJump 함수를 호출하게 하도록
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AKHU_GEBCharacter::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);



		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::Look);

		// Attack
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, AttackManager.Get(), &UAttackComponent::AttackStarted); // AttackManager.Get()으로 수정
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, AttackManager.Get(), &UAttackComponent::AttackTriggered); // AttackManager.Get()으로 수정
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, AttackManager.Get(), &UAttackComponent::AttackCompleted); // AttackManager.Get()으로 수정

		// Skill
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Started, this, &AKHU_GEBCharacter::SkillStart);
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Completed, this, &AKHU_GEBCharacter::SkillEnd);

		// 변신
		EnhancedInputComponent->BindAction(FormBase, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToBase);
		EnhancedInputComponent->BindAction(FormRange, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToRange);
		EnhancedInputComponent->BindAction(FormSwift, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToSwift);
		EnhancedInputComponent->BindAction(FormGuard, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToGuard);
		EnhancedInputComponent->BindAction(FormSpecial, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToSpecial);


		// 달리기
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AKHU_GEBCharacter::StartSprinting);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AKHU_GEBCharacter::StopSprinting);
		
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
		// 폼이 바뀔 때마다 OnFormChanged_Handler 함수를 호출하도록 연결합니다.
		FormManager->OnFormChanged.AddDynamic(this, &AKHU_GEBCharacter::OnFormChanged_Handler);
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
	if (GEngine && DamageCauser)
	{
		// 키(-1): 새 메시지가 기존 메시지를 덮어쓰지 않음
		// 시간(5.f): 5초간 화면에 표시
		// 색상(FColor::Red): 빨간색
		FString Msg = FString::Printf(TEXT("HIT! %s가 %s에게 %f 데미지를 받음!"),
			*GetName(), // 내 이름 (예: BP_KHUCharacter)
			*DamageCauser->GetName(), // 때린 액터 (예: BP_Tanker)
			Damage); // 받은 데미지

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, Msg);
	}
}

//StartJump 함수를 새로 구현합니다.
void AKHU_GEBCharacter::StartJump()
{
	bPlayerWantsToJump = true; // 애님 인스턴스가 읽어갈 플래그 ON
	Jump(); // ACharacter의 실제 점프 실행
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



//인터페이스 함수 4개를 파일 맨 아래에 새로 구현합니다.

float AKHU_GEBCharacter::GetAnimSpeed_Implementation() const
{
	// ACharacter의 기본 함수인 GetVelocity().Size()를 사용합니다.
	return GetVelocity().Size();
}

bool AKHU_GEBCharacter::GetAnimIsFalling_Implementation() const
{
	// ACharacter의 기본 함수인 GetCharacterMovement()->IsFalling()을 사용합니다.
	if (GetCharacterMovement())
	{
		return GetCharacterMovement()->IsFalling();
	}
	return false;
}

bool AKHU_GEBCharacter::GetAnimJumpInput_Implementation(bool bConsumeInput)
{
	// bPlayerWantsToJump 값을 읽고, 필요시 리셋(소모)합니다.
	const bool Result = bPlayerWantsToJump;
	if (bConsumeInput)
	{
		bPlayerWantsToJump = false; // 신호 리셋
	}
	return Result;
}


// [수정 후] GetAnimCharacterState_Implementation (컴포넌트 직접 쿼리)
ECharacterState AKHU_GEBCharacter::GetAnimCharacterState_Implementation() const
{
	// 1. 컴포넌트의 "공격/스킬" 상태를 우선적으로 확인합니다.
	// (SkillManager에 IsUsingSkill()과 같은 상태 변수가 있다고 가정합니다)
	if (SkillManager /*&& SkillManager->IsUsingSkill()*/) // TODO: SkillManager 상태 확인
	{
		return ECharacterState::Skill1;
	}

	// AttackManager의 blsAttacking 플래그를 확인합니다.
	if (AttackManager && AttackManager->bIsAttacking)
	{
		return ECharacterState::Attack;
	}

	// 2. 공격/스킬 상태가 아니라면, 캐릭터가 관리하는 기본 상태(Idle, Hit, Die)를 반환합니다.
	// CurrentPlayerState는 이제 Idle, Hit, Die 등만 관리합니다. [cite: 608]
	return CurrentPlayerState;
}

//달리기 관련 함수들
/** 폼이 변경될 때 호출되는 핸들러 */
void AKHU_GEBCharacter::OnFormChanged_Handler(EFormType NewForm, const UFormDefinition* Def)
{
	if (!Def || !GetCharacterMovement())
	{
		return;
	}

	// 1. 새 폼의 기본 속도를 DA에서 읽어와 변수에 저장합니다.
	CurrentFormBaseSpeed = Def->BaseWalkSpeed;

	// 2. 현재 상태(스프린트 중인지 여부)를 반영하여 속도를 즉시 업데이트합니다.
	UpdateMovementSpeed();
}

/** 스프린트 시작 (Shift 누름) */
void AKHU_GEBCharacter::StartSprinting(const FInputActionValue& Value)
{
	bIsSprinting = true;
	UpdateMovementSpeed();
}

/** 스프린트 종료 (Shift 뗌) */
void AKHU_GEBCharacter::StopSprinting(const FInputActionValue& Value)
{
	bIsSprinting = false;
	UpdateMovementSpeed();
}

/**
 * [핵심 로직]
 * 현재 폼의 기본 속도와 스프린트 여부에 따라
 * UCharacterMovementComponent의 MaxWalkSpeed를 설정합니다.
 */
void AKHU_GEBCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	if (bIsSprinting)
	{
		// 스프린트 중일 때
		EFormType CurrentForm = FormManager ? FormManager->CurrentForm : EFormType::Base;

		if (CurrentForm == EFormType::Swift)
		{
			// Swift 폼은 1200
			MoveComp->MaxWalkSpeed = 1400.f;
		}
		else
		{
			// 나머지 폼은 900
			MoveComp->MaxWalkSpeed = 900.f;
		}
	}
	else
	{
		// 스프린트 중이 아닐 때 (기본 속도 적용)
		MoveComp->MaxWalkSpeed = CurrentFormBaseSpeed;
	}
}