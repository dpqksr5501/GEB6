// Copyright Epic Games, Inc. All Rights Reserved.

#include "KHU_GEBCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "KHU_GEB.h"
#include "HealthComponent.h"
#include "ManaComponent.h"
#include "FormDefinition.h"
#include "FormManagerComponent.h"
#include "JumpComponent.h"
#include "LockOnComponent.h"
#include "AttackComponent.h"
#include "WeaponComponent.h"
#include "WeaponData.h"
#include "Skills/SkillManagerComponent.h"
#include "Skills/Skill_Range.h"
#include "Skills/Skill_Guard.h"
#include "StatManagerComponent.h"
#include "PotionControlComp.h"


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

	//적에게 가까울 시 카메라 충돌 해결
	CameraBoom->ProbeChannel = ECC_Visibility;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MeshRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRoot"));
	MeshRoot->SetupAttachment(RootComponent);
	GetMesh()->SetupAttachment(MeshRoot);

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	ManaComp = CreateDefaultSubobject<UManaComponent>(TEXT("ManaComp"));
	LockOnComp = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));

	FormManager = CreateDefaultSubobject<UFormManagerComponent>(TEXT("FormManager"));
	JumpManager = CreateDefaultSubobject<UJumpComponent>(TEXT("JumpManager"));
	AttackManager = CreateDefaultSubobject<UAttackComponent>(TEXT("AttackManager"));
	SkillManager = CreateDefaultSubobject<USkillManagerComponent>(TEXT("SkillManager"));
	StatManager = CreateDefaultSubobject<UStatManagerComponent>(TEXT("StatManager"));
	WeaponManager = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponManager"));

	PotionManagerComp = CreateDefaultSubobject<UPotionControlComp>(TEXT("PotionManagerComp"));

	static ConstructorHelpers::FObjectFinder<UInputAction> LOCKON(TEXT("/Script/EnhancedInput.InputAction'/Game/Input/Actions/IA_LockOn.IA_LockOn'"));
	if (LOCKON.Object) { LockOnAction = LOCKON.Object; }
	
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


	CurrentFormWalkSpeed = 600.f;
	CurrentFormSprintSpeed = 900.f;
	bIsSprinting = false;

	SkillSpeedMultiplier = 1.0f;

	//인터페이스용 변수 2개를 초기화합니다.
	CurrentPlayerState = ECharacterState::Idle;
	bPlayerWantsToJump = false;

	//Guard 스페이스바
	bSpaceActionInput = false;

	//Range 활강
	bIsRangeGliding = false;
}

void AKHU_GEBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		if (JumpManager)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, JumpManager, &UJumpComponent::HandleSpacePressed);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, JumpManager, &UJumpComponent::HandleSpaceReleased);
		}

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AKHU_GEBCharacter::Move);;

		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::Look);

		// LockOn
		if (LockOnAction && LockOnComp)
		{
			EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &AKHU_GEBCharacter::HandleLockOnToggle);
		}

		// Attack
		if (AttackManager)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, AttackManager.Get(), &UAttackComponent::AttackStarted);
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, AttackManager.Get(), &UAttackComponent::AttackTriggered);
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, AttackManager.Get(), &UAttackComponent::AttackCompleted);
		}

		// Skill
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Started, this, &AKHU_GEBCharacter::SkillStart);
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Completed, this, &AKHU_GEBCharacter::SkillEnd);
		EnhancedInputComponent->BindAction(UltimateAction, ETriggerEvent::Started, this, &AKHU_GEBCharacter::UltimateStart);
		EnhancedInputComponent->BindAction(UltimateAction, ETriggerEvent::Completed, this, &AKHU_GEBCharacter::UltimateEnd);

		// 변신
		EnhancedInputComponent->BindAction(FormBase, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToBase);
		EnhancedInputComponent->BindAction(FormRange, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToRange);
		EnhancedInputComponent->BindAction(FormSwift, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToSwift);
		EnhancedInputComponent->BindAction(FormGuard, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToGuard);
		EnhancedInputComponent->BindAction(FormSpecial, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToSpecial);

		// 달리기
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AKHU_GEBCharacter::StartSprinting);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AKHU_GEBCharacter::StopSprinting);

		//피격 테스트
		EnhancedInputComponent->BindAction(TestHitAction, ETriggerEvent::Started, this, &AKHU_GEBCharacter::OnTestHitInput);
	}
	else
	{
		UE_LOG(LogKHU_GEB, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AKHU_GEBCharacter::BeginPlay()
{
	Super::BeginPlay();

	OnTakeAnyDamage.AddDynamic(this, &AKHU_GEBCharacter::HandleAnyDamage);

	// 1. 카메라의 기본값(Idle 상태)을 변수에 저장합니다.
	if (CameraBoom)
	{
		DefaultCameraBoomLength = CameraBoom->TargetArmLength;//(기존 400.0f)
		TargetCameraBoomLength = DefaultCameraBoomLength;
	}
	if (FollowCamera)
	{
		DefaultFOV = FollowCamera->FieldOfView; // (기본 90.0f)
		TargetFOV = DefaultFOV;
		DefaultPostProcessSettings = FollowCamera->PostProcessSettings;

		//포스트 프로세스 효과의 기본값(0.0)을 목표 변수에 저장합니다.
		TargetFringeIntensity = 0.f; // (기본값 0)
		TargetVignetteIntensity = 0.f; // (기본값 0)
	}

	// 2. 델리게이트 바인딩
	if (FormManager)
	{
		FormManager->OnFormChanged.AddDynamic(this, &AKHU_GEBCharacter::OnFormChanged);
		FormManager->InitializeForms();

		if (StatManager && FormManager->FormSet)
		{
			StatManager->InitializeFromFormSet(FormManager->FormSet);
		}
	}
}

void AKHU_GEBCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 1. 카메라 붐(거리)을 부드럽게 보간
	if (CameraBoom && CameraBoom->TargetArmLength != TargetCameraBoomLength)
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(
			CameraBoom->TargetArmLength,
			TargetCameraBoomLength,
			DeltaTime,
			CameraInterpSpeed
		);
	}

	// 2. 카메라 FOV 및 포스트 프로세스 보간 (수정된 로직)
	if (FollowCamera)
	{
		// 2A. FOV 보간 (기존 코드)
		if (FollowCamera->FieldOfView != TargetFOV)
		{
			FollowCamera->FieldOfView = FMath::FInterpTo(
				FollowCamera->FieldOfView,
				TargetFOV,
				DeltaTime,
				CameraInterpSpeed
			);
		}

		// [!!! 2B. 포스트 프로세스 '강도'를 부드럽게 보간 (핵심 수정) !!!]

		// Scene Fringe (흐릿함)
		FollowCamera->PostProcessSettings.bOverride_SceneFringeIntensity = true; // [항상 덮어쓰기]
		FollowCamera->PostProcessSettings.SceneFringeIntensity = FMath::FInterpTo(
			FollowCamera->PostProcessSettings.SceneFringeIntensity, // 현재 값
			TargetFringeIntensity,                                  // 목표 값 (0.0 또는 2.0)
			DeltaTime,
			CameraInterpSpeed
		);

		// Vignette (어두움)
		FollowCamera->PostProcessSettings.bOverride_VignetteIntensity = true; // [항상 덮어쓰기]
		FollowCamera->PostProcessSettings.VignetteIntensity = FMath::FInterpTo(
			FollowCamera->PostProcessSettings.VignetteIntensity, // 현재 값
			TargetVignetteIntensity,                                 // 목표 값 (0.0 또는 0.8)
			DeltaTime,
			CameraInterpSpeed
		);
	}
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

void  AKHU_GEBCharacter::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// 1) Guard 스킬이 데미지 흡수 시도
	if (SkillManager)
	{
		if (USkillBase* ActiveSkill = SkillManager->Equipped.FindRef(ESkillSlot::Active))
		{
			if (USkill_Guard* GuardSkill = Cast<USkill_Guard>(ActiveSkill))
			{
				if (GuardSkill->HandleIncomingDamage(Damage, DamageType, InstigatedBy, DamageCauser))
				{
					UE_LOG(LogTemp, Log, TEXT("[Character] Damage absorbed by Guard"));
					return;
				}
			}
		}
	}

	if (!HealthComp || Damage <= 0.f) return;

	// 2) 진짜 공격자를 알아낸다 (컨트롤러의 Pawn → 없으면 DamageCauser)
	AActor* InstigatorActor = nullptr;
	if (InstigatedBy) { InstigatorActor = InstigatedBy->GetPawn(); }
	if (!InstigatorActor) { InstigatorActor = DamageCauser; }

	// === 3) 팀 체크: 적이 아니면 데미지 무시 ===
	if (!IsEnemyFor(InstigatorActor))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Character] Ignore damage from non-enemy: %s"),
			*GetNameSafe(InstigatorActor));
		return;
	}

	// 4) 여기까지 왔으면 '적'이 맞으므로 체력 감소
	const float FinalDamage = HealthComp->ApplyDamage(Damage, InstigatorActor, DamageCauser);

	// 5) 피격 리액션
	if (HealthComp->Health > 0.f && FinalDamage > 0.f) { PlayHitReaction(); }
}

void AKHU_GEBCharacter::Move(const FInputActionValue& Value)
{
	//Guard 끌어당기기 멈추기
	if (bIsMovementInputBlocked) return;

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// Range 조준 중이면, 이동 대신 스킬에 입력 전달
	if (SkillManager && SkillManager->IsRangeAiming())
	{
		if (SkillManager->GetActiveRangeSkill().IsValid())
		{
			SkillManager->GetActiveRangeSkill()->HandleAimMoveInput(MovementVector);
		}
		// 캐릭터는 실제로는 움직이지 않음
		return;
	}

	// 여기서부터는 일반 이동/락온 이동
	if (LockOnComp && LockOnComp->IsLockedOn())
	{
		// 1차 버전: 그냥 기존 DoMove 사용
		// (컨트롤러가 항상 타겟을 보고 있어서 서클링 느낌이 난다)
		DoMove(MovementVector.X, MovementVector.Y);
	}
	else
	{
		DoMove(MovementVector.X, MovementVector.Y);
	}
}

void AKHU_GEBCharacter::Look(const FInputActionValue& Value)
{

	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();


	//LockOn 상황일 때 마우스가 움질일 시 애니메이션이 깨지는 현상이 있음
	//해결하기 위해 LockOn시 마우스를 움직여서 바라보는 동작 제한.
	//그러나 위/아래로는 움직일 수 있게 해야함.
	if (LockOnComp && LockOnComp->IsLockedOn())
	{
		LookAxisVector.X = 0.0f; //좌/우 회전 차단
	}

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

void AKHU_GEBCharacter::UltimateStart(const FInputActionValue& Value)
{
	if (!SkillManager) return;
	SkillManager->TryActivate(ESkillSlot::Ultimate);
}

void AKHU_GEBCharacter::UltimateEnd(const FInputActionValue& Value)
{
	if (!SkillManager) return;
	SkillManager->TryStop(ESkillSlot::Ultimate);
}

void AKHU_GEBCharacter::SwitchToBase(const FInputActionValue& Value)
{
	if (SkillManager && SkillManager->IsFormChangeLocked()) return;
	if (!FormManager) return;
	FormManager->SwitchTo(EFormType::Base);
}

void AKHU_GEBCharacter::SwitchToRange(const FInputActionValue& Value)
{
	if (SkillManager && SkillManager->IsFormChangeLocked()) return;
	if (!FormManager) return;
	FormManager->SwitchTo(EFormType::Range);
}

void AKHU_GEBCharacter::SwitchToSwift(const FInputActionValue& Value)
{
	if (SkillManager && SkillManager->IsFormChangeLocked()) return;
	if (!FormManager) return;
	FormManager->SwitchTo(EFormType::Swift);
}

void AKHU_GEBCharacter::SwitchToGuard(const FInputActionValue& Value)
{
	if (SkillManager && SkillManager->IsFormChangeLocked()) return;
	if (!FormManager) return;
	FormManager->SwitchTo(EFormType::Guard);
}

void AKHU_GEBCharacter::SwitchToSpecial(const FInputActionValue& Value)
{
	if (SkillManager && SkillManager->IsFormChangeLocked()) return;
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
	if (JumpManager)
	{
		JumpManager->HandleSpacePressed();
	}
}

void AKHU_GEBCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	if (JumpManager)
	{
		JumpManager->HandleSpaceReleased();
	}
}

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

ECharacterState AKHU_GEBCharacter::GetAnimCharacterState_Implementation() const
{

	// AttackManager의 blsAttacking 플래그를 확인합니다.
	if (AttackManager && AttackManager->bIsAttacking)
	{
		return ECharacterState::Attack;
	}

	// 공격/스킬 상태가 아니라면, 캐릭터가 관리하는 기본 상태(Idle, Hit, Die)를 반환합니다.
	// CurrentPlayerState는 이제 Idle, Hit, Die 등만 관리합니다.
	return CurrentPlayerState;
}

bool AKHU_GEBCharacter::GetAnimSpaceActionInput_Implementation(bool bConsumeInput)
{
	const bool Result = bSpaceActionInput;
	if (bConsumeInput)
	{
		bSpaceActionInput = false; // 신호 소모(리셋)
	}
	return Result;
}

bool AKHU_GEBCharacter::GetAnimIsLockedOn_Implementation() const
{
	// 락온 컴포넌트가 유효하고, 락온 중이라면 true 반환
	if (LockOnComp && LockOnComp->IsLockedOn())
	{
		return true;
	}

	// (선택 사항) Range Aiming 중일 때도 스트레이프 모션(BS)을 쓰고 싶다면 아래 조건 추가
	// if (IsRangeAiming()) return true;

	return false;
}


/** 폼이 변경될 때 호출되는 핸들러 */
void AKHU_GEBCharacter::OnFormChanged(EFormType NewForm, const UFormDefinition* Def)
{
	if (JumpManager) { JumpManager->SetForm(NewForm, Def); }

	if (AttackManager) { AttackManager->SetForm(Def); }
	if (SkillManager) { SkillManager->EquipFromSkillSet(Def ? Def->SkillSet.Get() : nullptr); }

	if (WeaponManager) { WeaponManager->SetWeaponDefinition(Def ? Def->WeaponData.Get() : nullptr); }

	if (StatManager)
	{
		if (const FFormRuntimeStats* Stats = StatManager->GetStats(NewForm))
		{
			if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
			{
				CurrentFormWalkSpeed = Stats->WalkSpeed;
				CurrentFormSprintSpeed = Stats->SprintSpeed;
				MoveComp->MaxAcceleration = Stats->Acceleration;
			}

			// 나중에 데미지 줄 때 Stats->Attack, 맞을 때 Stats->Defense 사용
		}
	}

	// 2. 현재 상태(스프린트 중인지 여부)를 반영하여 속도를 즉시 업데이트합니다.
	UpdateMovementSpeed();

	// 3. 폼 변경 시 달리기 중이었다면, 카메라/효과를 새 폼에 맞게 재설정합니다.
	if (bIsSprinting)
	{
		if (NewForm == EFormType::Swift)
		{
			// Swift 폼으로 달리기 시작 (기존 코드)
			TargetCameraBoomLength = 550.f;
			TargetFOV = 105.f;

			//효과 목표값 설정
			TargetFringeIntensity = 2.0f;   // Swift는 강하게
			TargetVignetteIntensity = 0.8f; // Swift는 강하게
		}
		else
		{
			// 다른 폼으로 달리기 시작 (기존 코드)
			TargetCameraBoomLength = 450.f;
			TargetFOV = 95.f;

			//효과 목표값 설정
			TargetFringeIntensity = 1.0f;   // 일반 폼은 약하게
			TargetVignetteIntensity = 0.4f; // 일반 폼은 약하게
		}
	}

	//점프 횟수 설정(기본값 1)
	JumpMaxCount = Def->MaxJumpCount;

}

/** 스프린트 시작 (Shift 누름) */
void AKHU_GEBCharacter::StartSprinting(const FInputActionValue& Value)
{
	//락온 중이면 스프린트 불가
	if (LockOnComp && LockOnComp->IsLockedOn())
	{
		return;
	}

	bIsSprinting = true;
	UpdateMovementSpeed();

	// 1. 현재 폼 타입을 확인합니다.
	EFormType CurrentForm = FormManager ? FormManager->CurrentForm : EFormType::Base;

	// 2. 폼에 따라 목표 카메라 값을 설정합니다. (Tick에서 부드럽게 적용됨)
	if (CurrentForm == EFormType::Swift)
	{
		TargetCameraBoomLength = 550.f; // Swift 폼 스프린트 (예: 550)
		TargetFOV = 105.f;               // Swift 폼 FOV (예: 105)

		TargetFringeIntensity = 2.0f;
		TargetVignetteIntensity = 0.8f;
	}
	else
	{
		TargetCameraBoomLength = 450.f; // 일반 폼 스프린트 (예: 450)
		TargetFOV = 95.f;                // 일반 폼 FOV (예: 95)

		TargetFringeIntensity = 1.0f;
		TargetVignetteIntensity = 0.4f;
	}
}

/** 스프린트 종료 (Shift 뗌) */
void AKHU_GEBCharacter::StopSprinting(const FInputActionValue& Value)
{
	bIsSprinting = false;
	UpdateMovementSpeed();

	
	bool bIsLockedOn = (LockOnComp && LockOnComp->IsLockedOn());

	//카메라 값을 '기본값'으로 되돌립니다.
	if (!bIsLockedOn)
	{
		TargetCameraBoomLength = DefaultCameraBoomLength;
	}

	//화면 효과(FOV, 블러, 비네트)'는 락온 여부와 상관없이 무조건 끕니다.
	TargetFOV = DefaultFOV;
	TargetFringeIntensity = 0.f;
	TargetVignetteIntensity = 0.f;
}

void AKHU_GEBCharacter::UpdateMovementSpeed()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		float BaseSpeed = bIsSprinting ? CurrentFormSprintSpeed : CurrentFormWalkSpeed;

		// 스킬 배율 반영
		float FinalSpeed = BaseSpeed * SkillSpeedMultiplier;

		MoveComp->MaxWalkSpeed = FinalSpeed;
	}
}

void AKHU_GEBCharacter::SetSkillSpeedMultiplier(float InMultiplier)
{
	SkillSpeedMultiplier = InMultiplier;
	UpdateMovementSpeed();
}

/*Guard Form일 때 움직임 고정하는 함수*/
void AKHU_GEBCharacter::AutoResetSpaceAction()
{
	bSpaceActionInput = false;
}

/*Guard Form일 때 움직임 고정 해제하는 함수*/
void AKHU_GEBCharacter::ReleaseMovementLock()
{
	bIsMovementInputBlocked = false;
}

//활강 상태 확인하는 인터페이스함수 구현
bool AKHU_GEBCharacter::GetAnimIsRangeGliding_Implementation() const
{
	return bIsRangeGliding;
}


/////////////피격 테스트
void AKHU_GEBCharacter::OnTestHitInput(const FInputActionValue& Value)
{
	// 나 자신에게 데미지 10을 입힘 -> HandleAnyDamage가 호출됨
	UGameplayStatics::ApplyDamage(this, 10.0f, GetController(), this, UDamageType::StaticClass());
}

//피격 몽타주 재생
void AKHU_GEBCharacter::PlayHitReaction()
{
	//매니저 유효성 검사
	if (!FormManager) return;

	//현재 폼에 해당하는 데이터 에셋(FormDefinition) 찾기
	const UFormDefinition* Def = FormManager->FindDef(FormManager->CurrentForm);
	if (!Def) {	return;	}


	// 데이터가 없거나, 피격 몽타주가 비어있으면 리턴
	if (!Def || !Def->HitReactMontage) {return;}

	//애니메이션 인스턴스 가져오기
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		//설정된 단일 몽타주 재생
		// (에디터에서 이 몽타주가 'HitSlot' + 'Additive'로 설정되어 있어야 공격과 섞임)
		AnimInstance->Montage_Play(Def->HitReactMontage);
	}

	// 공격 중단(ResetComboHard) 호출 안 함 -> 공격 모션 유지하면서 움찔거림
}

void AKHU_GEBCharacter::HandleLockOnToggle()
{
	if (LockOnComp) { 
		LockOnComp->ToggleLockOn();

		//달리다가 락온 켜면, 즉시 걷기로 전환
		if (LockOnComp->IsLockedOn() && bIsSprinting)
		{
			FInputActionValue DummyValue; // 빈 값 생성
			StopSprinting(DummyValue);    // 스프린트 종료 함수 강제 호출
		}
	}
}

AActor* AKHU_GEBCharacter::GetLockOnTarget() const
{
	return (LockOnComp ? LockOnComp->GetCurrentTarget() : nullptr);
}

void AKHU_GEBCharacter::RefreshRotationMode()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	// 락온이 켜져 있거나, Range 조준 중이면 컨트롤러 Yaw를 기준으로 회전
	bool bShouldUseControllerYaw = false;

	if (LockOnComp && LockOnComp->IsLockedOn())
	{
		bShouldUseControllerYaw = true;
	}

	if (SkillManager && SkillManager->IsRangeAiming())
	{
		bShouldUseControllerYaw = true;
	}

	MoveComp->bOrientRotationToMovement = !bShouldUseControllerYaw;
	bUseControllerRotationYaw = bShouldUseControllerYaw;
}

bool AKHU_GEBCharacter::IsEnemyFor(const AActor* Other) const
{
	const AKHU_GEBCharacter* OtherChar = Cast<AKHU_GEBCharacter>(Other);
	if (OtherChar) return false;
	else return true;
}
