// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "TJump.generated.h"

class ACharacter;
class UJumpComponent;

/**
 * Enemy AI가 JumpComponent를 통해 점프를 수행하는 BTTaskNode
 * JumpComponent의 HandleSpacePressed를 호출하여 폼에 맞는 점프를 실행
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
	/** 캐시된 Character */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> CachedCharacter;

	/** 캐시된 JumpComponent */
	UPROPERTY(Transient)
	TObjectPtr<UJumpComponent> CachedJumpComp;

	/** 점프 시작 시간 (타임아웃 체크용) */
	float JumpStartTime;

	/** 2단 점프를 이미 실행했는지 여부 */
	bool bSecondJumpExecuted;

	// === 에디터에서 설정 가능한 프로퍼티 ===

	/** true면 최고점에서 2단 점프 자동 실행 (폼이 지원하는 경우) */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (DisplayName = "Enable Double Jump"))
	bool bEnableDoubleJump = false;

	/** 점프 타임아웃 시간 (초) - 이 시간 동안 착지하지 못하면 실패 */
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float JumpTimeout = 5.0f;
};
