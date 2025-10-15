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
	Skill1	UMETA(DisplayName = "Skill1"), //우클릭
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
	// 블루프린트에서 읽기 전용으로 사용할 변수들
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
