// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "MyAnimDataProvider.h" // 1. 생성한 인터페이스 헤더를 포함합니다.
#include "MonsterAnimInstanceBase.h" // 2. ECharacterState 열거형을 사용하기 위해 포함합니다.
#include "KHU_GEBCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UHealthComponent;
class UFormManagerComponent;
class UFormDefinition;
class UAttackComponent;
class USkillManagerComponent;

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

	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* AttackAction;
	UPROPERTY(EditAnywhere, Category = "Input") UInputAction* SkillAction;

	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormBase;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormRange;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormSwift;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormGuard;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* FormSpecial;

public:
	/** Constructor */
	AKHU_GEBCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFormManagerComponent* FormManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAttackComponent* AttackManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkillManagerComponent* SkillManager;


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



	//점프 입력을 받을 새 C++ 함수를 선언합니다 (BP의 DoJumpStart 대신).
	void StartJump();
	/** AttackComponent로 입력을 전달하는 것과 *별개로* 캐릭터의 상태를 관리합니다. */
	void AttackStarted_Character(const FInputActionValue& Value);
	void AttackCompleted_Character(const FInputActionValue& Value);

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
	void HandleAnyDamage(AActor* DamagedActor, float Damage,
		const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	UFUNCTION()
	void OnFormChanged(EFormType NewForm, const UFormDefinition* Def);


	//인터페이스(IMyAnimDataProvider)의 함수 4개를 구현하겠다고 선언합니다.
	virtual float GetAnimSpeed_Implementation() const override;
	virtual ECharacterState GetAnimCharacterState_Implementation() const override;
	virtual bool GetAnimIsFalling_Implementation() const override;
	virtual bool GetAnimJumpInput_Implementation(bool bConsumeInput) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

