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

	// 상태를 안전하게 가져갈 수 있는 public 함수
	UFUNCTION(BlueprintPure, Category = "State")
	ECharacterState GetCharacterState() const { return CharacterState; }

	// 상태를 안전하게 변경할 수 있는 public 함수
	UFUNCTION(BlueprintCallable, Category = "State")
	void SetCharacterState(ECharacterState NewState);

protected:
	// 상태 변수는 외부에서 직접 건드리지 못하도록 protected로 보호합니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	ECharacterState CharacterState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionMove;

	// 블루프린트에서 IA_MouseLook 애셋을 지정할 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionLook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionShift;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* InputMappingContext;

	// 블루프린트에서 IA_Jump 애셋을 지정할 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionJump;

	// 블루프린트에서 IA_Ctrl 애셋을 지정할 변수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* InputActionCtrl;

	
	//IA_Move 입력 발생 시 호출되는 함수
	void Move(const FInputActionValue& Value);

	// IA_MouseLook 입력이 발생했을 때 호출될 함수
	void Look(const FInputActionValue& Value);

	// 쉬프트 액션을 시작할 때 호출될 함수 (에디터에서 재정의 가능)
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void StartShiftAction();

	// 쉬프트 액션을 멈출 때 호출될 함수 (virtual 추가)
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void StopShiftAction();

	// 점프 키 액션을 처리할 함수들 (에디터에서 재정의할 수 있도록)
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void JumpAction_Start();   // 처음 눌렀을 때
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void JumpAction_Triggered(); // 누르고 있을 때
	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void JumpAction_Stop();    // 뗐을 때

	// 컨트롤 키 액션을 처리할 함수
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
