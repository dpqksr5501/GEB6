// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MonsterAnimInstanceBase.generated.h"

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle	UMETA(DisplayName = "Idle"),
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

	//회전 시 기울기에 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	float YawDeltaSpeed;

	//좌/우 각도
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|AimOffset")
	float AimYaw;

	//위/아래 각도
	/** 캐릭터가 바라보는 Pitch (위/아래) 각도 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|AimOffset")
	float AimPitch;

protected:
	UPROPERTY()
	AMonsterBase* OwningMonster;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	float LastYaw;//마지막 Yaw값 가져오기
};
