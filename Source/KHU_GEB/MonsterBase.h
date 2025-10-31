// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "MonsterAnimInstanceBase.h"
#include "MonsterBase.generated.h"


UCLASS()
class KHU_GEB_API AMonsterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AMonsterBase();

	//// ü�°���
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UHealthComponent* HealthComp;

	/** �������Ʈ���� ���� ü�� �������� */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const;

	/** �������Ʈ���� ü�� ȸ���ϱ� */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);





	///////////

	// ���¸� �����ϰ� ������ �� �ִ� public �Լ�
	UFUNCTION(BlueprintPure, Category = "State")
	ECharacterState GetCharacterState() const { return CharacterState; }

	// ���¸� �����ϰ� ������ �� �ִ� public �Լ�
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetCharacterState(ECharacterState NewState);

protected:
	// ���� ������ �ܺο��� ���� �ǵ帮�� ���ϵ��� protected�� ��ȣ�մϴ�.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	ECharacterState CharacterState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionMove;

	// �������Ʈ���� IA_MouseLook �ּ��� ������ ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionLook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionShift;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* InputMappingContext;

	// �������Ʈ���� IA_Jump �ּ��� ������ ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionJump;

	// �������Ʈ���� IA_Ctrl �ּ��� ������ ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionCtrl;

	//�����Ϳ��� IA_LAttack�� ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionLAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionRAttack;

	///////////////////////////////////////////

	
	//IA_Move �Է� �߻� �� ȣ��Ǵ� �Լ�
	void Move(const FInputActionValue& Value);

	// IA_MouseLook �Է��� �߻����� �� ȣ��� �Լ�
	void Look(const FInputActionValue& Value);

	// ����Ʈ �׼��� ������ �� ȣ��� �Լ� (�����Ϳ��� ������ ����)
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void StartShiftAction();

	// ����Ʈ �׼��� ���� �� ȣ��� �Լ� (virtual �߰�)
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void StopShiftAction();

	// ���� Ű �׼��� ó���� �Լ��� (�����Ϳ��� �������� �� �ֵ���)
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void JumpAction_Start();   // ó�� ������ ��
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void JumpAction_Triggered(); // ������ ���� ��
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void JumpAction_Stop();    // ���� ��

	// ��Ʈ�� Ű �׼��� ó���� �Լ�
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void CtrlAction_Start();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Event")
	void OnLandedEvent(const FHitResult& Hit);

	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void LAttackAction_Start();
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void RAttackAction_Start();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;

	virtual void Landed(const FHitResult& Hit) override;

};
