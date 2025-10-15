// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MonsterAnimInstanceBase.generated.h"

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle	UMETA(DisplayName = "Idle"),
	Attack	UMETA(DisplayName = "Attack"),
	Skill1	UMETA(DisplayName = "Skill1"), //��Ŭ��
	Hit		UMETA(DisplayName = "Hit"),
	Die		UMETA(DisplayName = "Die")
};

class AMonsterBase;
/**
 * 
 */
UCLASS()
class KHU_GEB_API UMonsterAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	// �������Ʈ���� �б� �������� ����� ������
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	float Speed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	ECharacterState CharacterState;

protected:
	UPROPERTY()
	AMonsterBase* OwningMonster;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
