// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "TGlide.generated.h"

class ACharacter;
class UJumpComponent;
class UEnemyAnimIntance;

/**
 * Archer(Range 폼)가 글라이드(활강)를 수행하는 BTTaskNode
 * JumpComponent의 HandleSpacePressed를 호출하여 높이 점프 후 천천히 하강
 * 착지할 때까지 InProgress 상태 유지
 */
UCLASS()
class KHU_GEB_API UTGlide : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTGlide();

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

	/** 캐시된 AnimInstance */
	UPROPERTY(Transient)
	TObjectPtr<UEnemyAnimIntance> CachedAnimInstance;

	/** 글라이드 시작 시간 (타임아웃 체크용) */
	float GlideStartTime;

	// === 에디터에서 설정 가능한 프로퍼티 ===

	/** 글라이드 타임아웃 시간 (초) - 이 시간 동안 착지하지 못하면 실패 */
	UPROPERTY(EditAnywhere, Category = "Glide", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float GlideTimeout = 10.0f;

	// 글라이드 노드에 선택 플래그를 만들어줘. 글라이드 하다가 화염구를 쏘게할 거야.
	UPROPERTY(EditAnywhere, Category = "Fireball")
	bool bCanFireballDuringGlide = false;
};
