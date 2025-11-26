// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "TJump.generated.h"

class UJumpComponent;
class AEnemy_Base;
class UMonsterAnimInstanceBase;

/**
 * Enemy AI가 JumpComponent를 사용하여 점프를 수행하는 BTTaskNode
 * Enemy의 DefaultFormDef에 설정된 폼으로 점프 실행
 */
UCLASS()
class KHU_GEB_API UTJump : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTJump();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 캐시된 JumpComponent (ExecuteTask에서 설정) */
	UPROPERTY(Transient)
	TObjectPtr<UJumpComponent> CachedJumpComp;

	/** 캐시된 Enemy_Base (ExecuteTask에서 설정) */
	UPROPERTY(Transient)
	TObjectPtr<AEnemy_Base> CachedEnemy;

	/** 캐시된 AnimInstance */
	UPROPERTY(Transient)
	TObjectPtr<UMonsterAnimInstanceBase> CachedAnimInstance;

	/** 점프 시작 시간 (타임아웃 체크용) */
	float JumpStartTime;

	/** 점프 타임아웃 시간 (초) - 이 시간 동안 착지하지 못하면 실패 */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float JumpTimeout = 5.0f;
};
