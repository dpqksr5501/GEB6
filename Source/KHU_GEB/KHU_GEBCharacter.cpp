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
#include "PlayerStatsComponent.h"
#include "HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "KHU_GEB.h"


AKHU_GEBCharacter::AKHU_GEBCharacter()
{	

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

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

	// 스탯 컴포넌트 생성 (BeginPlay의 Stats 사용을 보장)
	Stats = CreateDefaultSubobject<UPlayerStatsComponent>(TEXT("PlayerStats"));

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
		EnhancedInputComponent->BindAction(RangeForm, ETriggerEvent::Triggered, this, &AKHU_GEBCharacter::SwitchToBase);
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


void  AKHU_GEBCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 폼별 스탯 초기화: DefaultFormStats 맵을 Stats->FormStats로 이식
	if (Stats)
	{
		Stats->FormStats.Empty();
		for (const auto& Pair : DefaultFormStats)
		{
			FFormStatState S; S.StatsData = Pair.Value;
			Stats->FormStats.Add(Pair.Key, S);
		}
		// 기본 체력
		if (Stats->MaxHealth <= 0.f) Stats->MaxHealth = 120.f;
		if (Stats->Health <= 0.f) Stats->Health = Stats->MaxHealth;

		// 모든 폼 재계산(캐시 업데이트)
		for (const auto& Pair : Stats->FormStats)
		{
			Stats->RecalcForForm(Pair.Key);
		}
		// 시작 폼의 실효 이속/쿨다운 반영
		ApplyStatsForCurrentForm();
	}

	OnTakeAnyDamage.AddDynamic(this, &AKHU_GEBCharacter::HandleAnyDamage);
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

void AKHU_GEBCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);



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
	if (CurrentForm == EPlayerForm::None) return;
	SwitchTo(EPlayerForm::None);
}

void AKHU_GEBCharacter::SwitchToRange(const FInputActionValue& Value)
{
	if (CurrentForm == EPlayerForm::Range) return;
	SwitchTo(EPlayerForm::Range);
}

void AKHU_GEBCharacter::SwitchToSpeed(const FInputActionValue& Value)
{
	if (CurrentForm == EPlayerForm::Speed) return;
	SwitchTo(EPlayerForm::Speed);
}

void AKHU_GEBCharacter::SwitchToDefense(const FInputActionValue& Value)
{
	if (CurrentForm == EPlayerForm::Defense) return;
	SwitchTo(EPlayerForm::Defense);
}

void AKHU_GEBCharacter::SwitchToDebuff(const FInputActionValue& Value)
{
	if (CurrentForm == EPlayerForm::Debuff) return;
	SwitchTo(EPlayerForm::Debuff);
}

FVector AKHU_GEBCharacter::FindSafeGroundLocation(const FVector& Around) const
{
	FHitResult Hit;
	FVector Start = Around + FVector(0, 0, 1000);
	FVector End = Around - FVector(0, 0, 2000);
	FCollisionQueryParams P(SCENE_QUERY_STAT(FormGroundTrace), false, this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, P))
	{
		const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		return Hit.Location + FVector(0, 0, HalfHeight + 2.f);
	}
	return Around;
}

void AKHU_GEBCharacter::PlaySmallFade(APlayerController* PC, float Time) const
{
	if (!PC) return;
	if (APlayerCameraManager* CM = PC->PlayerCameraManager)
	{
		CM->StartCameraFade(0.f, 1.f, Time, FLinearColor::Black, false, true);
		CM->StartCameraFade(1.f, 0.f, Time, FLinearColor::Black, false, true);
	}
}

void AKHU_GEBCharacter::ApplyStatsForCurrentForm()
{
	if (!Stats) return;
	if (auto* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Stats->GetMoveSpeed(CurrentForm);
	}
	// 스킬 시스템이 있다면 여기서 쿨타임 반영:
	// AbilitySystem->SetGlobalCooldown( Stats->GetSkillCooldown(CurrentForm) );
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

void AKHU_GEBCharacter::SwitchTo(EPlayerForm NewForm)
{
	if (NewForm == CurrentForm) return;

	// 새 폼 클래스 확인
	TSubclassOf<AKHU_GEBCharacter>* Cls = FormClasses.Find(NewForm);
	if (!Cls || !(*Cls)) return;

	// 1) 상태 번들 추출
	const FPlayerStateBundle Bundle = MakeStateBundle();

	// 2) 스폰 트랜스폼(지면 보정)
	FTransform SpawnTM = GetActorTransform();
	SpawnTM.SetLocation(FindSafeGroundLocation(SpawnTM.GetLocation()));

	// 3) 새 Pawn 생성
	AKHU_GEBCharacter* NewPawn = GetWorld()->SpawnActorDeferred<AKHU_GEBCharacter>(*Cls, SpawnTM);
	UGameplayStatics::FinishSpawningActor(NewPawn, SpawnTM);

	// 4) 상태 이관 + 폼 지정 + 스탯 즉시 반영
	NewPawn->ApplyStateBundle(Bundle);
	NewPawn->CurrentForm = NewForm;
	NewPawn->ApplyStatsForCurrentForm();

	// 5) 포제션 + 페이드
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PlaySmallFade(PC, 0.08f);
		PC->UnPossess();
		PC->Possess(NewPawn);
		PlaySmallFade(PC, 0.08f);
	}

	// 6) 기존 Pawn 제거
	Destroy();
}

FPlayerStateBundle AKHU_GEBCharacter::MakeStateBundle() const
{
	FPlayerStateBundle B;
	if (Stats)
	{
		B.MaxHP = Stats->MaxHealth;
		B.HP = FMath::Clamp(Stats->Health, 0.f, Stats->MaxHealth);
		// 예시: Ammo/ReserveAmmo 등 필요 시 채우세요
		// B.ActiveEffects ... 필요 시 채우기

		// 폼 업그레이드 진행도 저장
		for (const auto& Pair : Stats->FormStats)
		{
			const EPlayerForm Form = Pair.Key;
			const FFormStatState& S = Pair.Value;

			FFormStatProgressSave Save;
			Save.Form = Form;
			Save.AttackLevel = S.Progress.AttackLevel;
			Save.DefenseLevel = S.Progress.DefenseLevel;
			Save.MoveSpeedLevel = S.Progress.MoveSpeedLevel;
			Save.SkillCooldownLevel = S.Progress.SkillCooldownLevel;
			B.FormProgresses.Add(Save);
		}
	}
	return B;
}

void AKHU_GEBCharacter::ApplyStateBundle(const FPlayerStateBundle& B)
{
	if (!Stats) return;

	Stats->MaxHealth = B.MaxHP;
	Stats->Health = FMath::Clamp(B.HP, 0.f, B.MaxHP);

	// 폼 진행도 복원 + 재계산
	for (const FFormStatProgressSave& S : B.FormProgresses)
	{
		if (FFormStatState* Target = Stats->FormStats.Find(S.Form))
		{
			Target->Progress.AttackLevel = S.AttackLevel;
			Target->Progress.DefenseLevel = S.DefenseLevel;
			Target->Progress.MoveSpeedLevel = S.MoveSpeedLevel;
			Target->Progress.SkillCooldownLevel = S.SkillCooldownLevel;
			Stats->RecalcForForm(S.Form);
		}
	}
}
