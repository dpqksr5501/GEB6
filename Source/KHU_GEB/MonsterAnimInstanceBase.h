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

class AMonsterBase; // 전방 선언
class IMyAnimDataProvider; // 1. 인터페이스 전방 선언 추가

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

	//좌/우 방향의 delta값을 가져옴
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	float YawDeltaSpeed;

	/** 캐릭터가 방금 점프 입력을 했는지 여부 (신호용) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Movement")
	bool bJumpInput_Anim;

	//캐릭터가 추락하는지 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Movement")
	bool bIsFalling;


	//여기 변수들은 (AimYaw, AimPitch) 에임 오프셋을 쓸 수 있을 때를 대비해서 만들어뒀습니다.
	//에임 좌/우
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|AimOffset")
	float AimYaw;

	//에임 상/하
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|AimOffset")
	float AimPitch;

protected:
	//UPROPERTY()
	//AMonsterBase* OwningMonster;

	//인터페이스 포인터를 저장할 변수로 변경합니다.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Owner")
	TScriptInterface<IMyAnimDataProvider> OwningDataProvider;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	float LastYaw;	//delta yaw를 구하기 위해서
};
