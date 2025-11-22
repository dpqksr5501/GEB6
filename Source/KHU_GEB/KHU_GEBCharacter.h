// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MyAnimDataProvider.h" // 1. 생성한 인터페이스 헤더를 포함합니다.
#include "MonsterAnimInstanceBase.h" // 2. ECharacterState 열거형을 사용하기 위해 포함합니다.
#include "FormDefinition.h"
#include "Camera/CameraComponent.h"
#include "NiagaraComponent.h"
#include "KHU_GEBCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USceneComponent;
class UInputAction;
struct FInputActionValue;
class UHealthComponent;
class UManaComponent;
class UFormManagerComponent;
class UFormDefinition;
class UJumpComponent;
class UAttackComponent;
class USkillManagerComponent;
class UStatManagerComponent;
class UNiagaraComponent;
class UWeaponComponent; //[추가] WeaponComponent 전방 선언


DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(abstract)
class AKHU_GEBCharacter : public ACharacter, public IMyAnimDataProvider //상속을 추가
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Swift 폼으로 달릴 때 활성화될 나이아가라 이펙트입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UNiagaraComponent* SwiftSprintVFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	USceneComponent* MeshRoot;

protected:
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Attack Input Action */
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* AttackAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* SkillAction;

	/** Transform Input Action */
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormBase;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormRange;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormSwift;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormGuard;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormSpecial;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;

private:
	/** 현재 폼의 기본 이동 속도 */
	float CurrentFormWalkSpeed;
	float CurrentFormSprintSpeed;

	/** 현재 스프린트 중인지 여부 */
	bool bIsSprinting;

	/** 스킬로 인한 이동속도 배율 (1.0이 기본) */
	float SkillSpeedMultiplier = 1.0f;

	/** 카메라 효과를 부드럽게 보간(Interp)하는 속도입니다. */
	UPROPERTY(EditAnywhere, Category = "Camera Effects")
	float CameraInterpSpeed = 10.0f;

	// --- 카메라 붐(거리) 변수 ---
	/** BeginPlay에서 저장되는 카메라 붐의 기본(Idle) 거리입니다. */
	float DefaultCameraBoomLength;
	/** Tick 함수가 매 프레임 도달하려는 목표 카메라 붐 거리입니다. */
	float TargetCameraBoomLength;

	// --- 카메라 FOV(광각) 변수 ---
	/** BeginPlay에서 저장되는 카메라의 기본(Idle) FOV 값입니다. */
	float DefaultFOV;
	/** Tick 함수가 매 프레임 도달하려는 목표 FOV 값입니다. */
	float TargetFOV;

	// --- 포스트 프로세스(이펙트) 변수 ---
	/** BeginPlay에서 저장되는 카메라의 기본 포스트 프로세스 설정입니다. (이펙트 끄기용) */
	UPROPERTY() // UPROPERTY로 GC가 참조하게 함
	FPostProcessSettings DefaultPostProcessSettings;

	/** Tick 함수가 매 프레임 도달하려는 목표 SceneFringe(흐릿함) 강도입니다. */
	float TargetFringeIntensity;

	/** Tick 함수가 매 프레임 도달하려는 목표 Vignette(어두움) 강도입니다. */
	float TargetVignetteIntensity;

public:
	/** Constructor */
	AKHU_GEBCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UManaComponent* ManaComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFormManagerComponent* FormManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UJumpComponent* JumpManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAttackComponent> AttackManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkillManagerComponent* SkillManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStatManagerComponent* StatManager;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWeaponComponent> WeaponManager;

	//추가 코드 부분입니다.
	//애님 인스턴스에 데이터를 제공할 플레이어 전용 변수 2개를 추가합니다.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	ECharacterState CurrentPlayerState; // 몬스터의 CharacterState와 동일한 역할

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "State")
	bool bPlayerWantsToJump; // 몬스터의 bJumpInput과 동일한 역할

protected:
	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for Skill */
	void SkillStart(const FInputActionValue& Value);
	void SkillEnd(const FInputActionValue& Value);

	/** Called for 변신 */
	void SwitchToBase(const FInputActionValue& Value);
	void SwitchToRange(const FInputActionValue& Value);
	void SwitchToSwift(const FInputActionValue& Value);
	void SwitchToGuard(const FInputActionValue& Value);
	void SwitchToSpecial(const FInputActionValue& Value);


	/** 스프린트 입력을 받았을 때 호출됩니다. (Started) */
	void StartSprinting(const FInputActionValue& Value);

	/** 스프린트 입력을 뗐을 때 호출됩니다. (Completed) */
	void StopSprinting(const FInputActionValue& Value);

	/** 폼이 변경되었을 때 FormManager로부터 호출됩니다. */
	UFUNCTION()
	void OnFormChanged(EFormType NewForm, const UFormDefinition* Def);

	/** 현재 상태(폼, 스프린트 여부)에 맞춰 이동 속도를 업데이트합니다. */
	void UpdateMovementSpeed();
	

public:
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);


	//인터페이스(IMyAnimDataProvider)의 함수 4개를 구현하겠다고 선언합니다.
	virtual float GetAnimSpeed_Implementation() const override;
	virtual ECharacterState GetAnimCharacterState_Implementation() const override;
	virtual bool GetAnimIsFalling_Implementation() const override;
	virtual bool GetAnimJumpInput_Implementation(bool bConsumeInput) override;

	/** 스킬에 의해 적용되는 이동속도 배율을 설정합니다. (1.0 = 기본속도) */
	void SetSkillSpeedMultiplier(float InMultiplier);
	float GetSkillSpeedMultiplier() const { return SkillSpeedMultiplier; }

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE USceneComponent* GetMeshRoot() const { return MeshRoot; }

private:
	bool bIsRangeAiming = false;
	TWeakObjectPtr<class USkill_Range> ActiveRangeSkill;

public:
	void OnRangeAimingStarted(USkill_Range* Skill);
	void OnRangeAimingEnded(USkill_Range* Skill);
	bool IsRangeAiming() const { return bIsRangeAiming; }
};

