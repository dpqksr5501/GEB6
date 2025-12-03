// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "TTaunt.generated.h"

class ACharacter;
class UJumpComponent;

/**
 * Tanker(Guard 폼)가 도발(Taunt) 행동을 수행하는 BTTaskNode
 * JumpComponent의 HandleGuardPressed를 호출하여 주변 적들을 끌어당김
 * Guard 끌어당김이 완료될 때까지 대기
 */
UCLASS()
class KHU_GEB_API UTTaunt : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTTaunt();

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

	/** 도발 시작 시간 (완료 체크용) */
	float TauntStartTime;

	// === 에디터에서 설정 가능한 프로퍼티 ===

	/** 도발(끌어당김) 지속 시간 (초) - JumpComponent의 GuardPullDuration과 동기화 필요 */
	UPROPERTY(EditAnywhere, Category = "Taunt", meta = (ClampMin = "0.5", ClampMax = "5.0"))
	float TauntDuration = 2.1f; // Guard 끌어당김 시간(2.0) + 여유(0.1)
};
