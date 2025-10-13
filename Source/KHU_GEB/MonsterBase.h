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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;

	virtual void Landed(const FHitResult& Hit) override;

};
