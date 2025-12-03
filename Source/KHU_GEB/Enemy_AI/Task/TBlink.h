// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "TBlink.generated.h"

class ACharacter;
class UJumpComponent;

/**
 * Assassin(Special 폼)이 블링크(순간이동)를 수행하는 BTTaskNode
 * JumpComponent의 HandleSpacePressed를 호출하여 앞으로 순간이동 실행
 * 즉시 완료되는 타입 (InProgress 없이 바로 Succeeded/Failed 반환)
 */
UCLASS()
class KHU_GEB_API UTBlink : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTBlink();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	/** 캐시된 Character */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> CachedCharacter;

	/** 캐시된 JumpComponent */
	UPROPERTY(Transient)
	TObjectPtr<UJumpComponent> CachedJumpComp;
};
