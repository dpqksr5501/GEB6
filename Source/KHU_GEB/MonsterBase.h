// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;

};
