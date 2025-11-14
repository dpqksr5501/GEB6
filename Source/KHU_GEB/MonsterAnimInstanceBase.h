// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MonsterAnimInstanceBase.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class USkeletalMeshComponent;
class AMonsterBase; // 전방 선언
class IMyAnimDataProvider; // 1. 인터페이스 전방 선언 추가

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
	Idle	UMETA(DisplayName = "Idle"),
	Attack	UMETA(DisplayName = "Attack"),
	Skill1	UMETA(DisplayName = "Skill1"), //우클릭
	Hit		UMETA(DisplayName = "Hit"),
	Die		UMETA(DisplayName = "Die")
};



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


	/**
	 * [에디터에서 설정]
	 * 이 애님 BP가 달릴 때 사용할 파티클 시스템(Cascade) 애셋입니다.
	 * 'EditDefaultsOnly'는 자식 ABP의 '클래스 디폴트'에서만 설정 가능하게 합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	UNiagaraSystem* SprintEffectTemplate;

	/**
	 * [에디터에서 설정]
	 * 이펙트를 부착할 스켈레탈 메쉬 소켓 이름입니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FName SprintEffectSocketName;

	/**
	 * [C++ 내부용]
	 * 현재 활성화된 이펙트 컴포넌트를 추적하여, 중복 생성을 막고 파괴할 때 사용합니다.
	 * 'Transient'는 저장되지 않는 런타임 전용 변수임을 의미합니다.
	 */
	UPROPERTY(Transient)
	UNiagaraComponent* ActiveSprintEffectComponent;

	/**
	 * [블루프린트(ABP)에서 호출]
	 * 이펙트를 켜거나 끄는 함수입니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Effects")
	void ToggleSprintEffect(bool bActivate, USkeletalMeshComponent* TargetMesh);
	
};
