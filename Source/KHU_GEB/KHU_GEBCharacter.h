// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "PlayerEnums.h"
#include "FormStatsData.h"
#include "PlayerStateBundle.h"
#include "KHU_GEBCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UPlayerStatsComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AKHU_GEBCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	

public:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage,
		const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UHealthComponent* HealthComp;

	/** ��������Ʈ���� ���� ü�� �������� */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const;

	/** ��������Ʈ���� ü�� ȸ���ϱ� */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);


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

	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* BaseForm;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* RangeForm;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* SpeedForm;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* DefenseForm;
	UPROPERTY(EditAnywhere, Category = "Input|Forms") UInputAction* DebuffForm;

public:
	/** Constructor */
	AKHU_GEBCharacter();

	// === 폼 상태 ===
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Forms")
	EPlayerForm CurrentForm = EPlayerForm::None;

	// 폼 → 폼용 블루프린트 클래스 (에디터에서 BP 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forms")
	TMap<EPlayerForm, TSubclassOf<AKHU_GEBCharacter>> FormClasses;

	// 폼 → 폼 스탯 DataAsset (에디터에서 4개 DataAsset 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Forms")
	TMap<EPlayerForm, TSoftObjectPtr<UFormStatsData>> DefaultFormStats;

	// 스탯(공통 소유자)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	UPlayerStatsComponent* Stats;

protected:
	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for 변신 */
	void SwitchToBase(const FInputActionValue& Value);
	void SwitchToRange(const FInputActionValue& Value);
	void SwitchToSpeed(const FInputActionValue& Value);
	void SwitchToDefense(const FInputActionValue& Value);
	void SwitchToDebuff(const FInputActionValue& Value);

	// 전환 시 지면 안전 위치 찾기
	FVector FindSafeGroundLocation(const FVector& Around) const;

	// 짧은 페이드
	void PlaySmallFade(APlayerController* PC, float Time = 0.08f) const;

	// 현재 폼 스탯을 실제 플레이에 반영(이속/쿨타임 등)
	void ApplyStatsForCurrentForm();

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

	// 폼 전환 API
	UFUNCTION(BlueprintCallable, Category = "Forms") void SwitchTo(EPlayerForm NewForm);

	// 번들 직렬화/복원
	UFUNCTION(BlueprintCallable, Category = "State") FPlayerStateBundle MakeStateBundle() const;
	UFUNCTION(BlueprintCallable, Category = "State") void ApplyStateBundle(const FPlayerStateBundle& B);

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

